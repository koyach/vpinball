#include "stdafx.h"

HitFlipper::HitFlipper(const float x, const float y, float baser, float endr, float flipr, const float angle,
                       const float zlow, const float zhigh, float strength, const float mass)
{	
   m_flipperanim.m_height = zhigh - zlow;
   m_flipperanim.m_fCompatibility=fTrue;

   m_flipperanim.m_hitcircleBase.m_pfe = NULL;
   m_flipperanim.m_hitcircleEnd.m_pfe = NULL;

   m_flipperanim.m_lineseg1.m_pfe = NULL;
   m_flipperanim.m_lineseg2.m_pfe = NULL;

   m_flipperanim.m_lineseg1.m_rcHitRect.zlow = zlow;
   m_flipperanim.m_lineseg1.m_rcHitRect.zhigh = zhigh;

   m_flipperanim.m_lineseg2.m_rcHitRect.zlow = zlow;
   m_flipperanim.m_lineseg2.m_rcHitRect.zhigh = zhigh;

   m_flipperanim.m_hitcircleEnd.zlow = zlow;
   m_flipperanim.m_hitcircleEnd.zhigh = zhigh;

   m_flipperanim.m_hitcircleBase.zlow = zlow;
   m_flipperanim.m_hitcircleBase.zhigh = zhigh;

   m_flipperanim.m_hitcircleBase.center.x = x;
   m_flipperanim.m_hitcircleBase.center.y = y;

   if (baser < 0.01f) baser = 0.01f; // must not be zero 
   m_flipperanim.m_hitcircleBase.radius = baser; //radius of base section

   if (endr < 0.01f) endr = 0.01f; // must not be zero 
   m_flipperanim.m_endradius = endr;		// radius of flipper end

   if (flipr < 0.01f) flipr = 0.01f; // must not be zero 
   m_flipperanim.m_flipperradius = flipr;	//radius of flipper arc, center-to-center radius

   m_flipperanim.m_angleCur = angle;
   m_flipperanim.m_angleEnd = angle;

   m_flipperanim.m_anglespeed = 0;

   const float fa = asinf((baser-endr)/flipr); //face to centerline angle (center to center)

   m_flipperanim.faceNormOffset = (float)(M_PI/2.0) - fa; //angle of normal when flipper center line at angle zero

   m_flipperanim.SetObjects(angle);	

   m_flipperanim.m_fAcc = 0;
   m_flipperanim.m_mass = mass;

   m_last_hittime = 0;

   m_flipperanim.m_force = 2;
   if (strength < 0.01f) strength = 0.01f;
   m_forcemass = strength;

   m_flipperanim.m_maxvelocity = m_flipperanim.m_force * 4.5f;

   m_flipperanim.m_lastHitFace = false; // used to optimize hit face search order

   const float len = m_flipperanim.m_flipperradius*cosf(fa); //Cosine of face angle X hypotenuse
   m_flipperanim.m_lineseg1.length = len;
   m_flipperanim.m_lineseg2.length = len;

   m_flipperanim.zeroAngNorm.x =  sinf(m_flipperanim.faceNormOffset);// F2 Norm, used in Green's transform, in FPM time search
   m_flipperanim.zeroAngNorm.y = -cosf(m_flipperanim.faceNormOffset);// F1 norm, change sign of x component, i.e -zeroAngNorm.x

#if 0 // needs wiring of moment of inertia
   // now calculate moment of inertia using isoceles trapizoid and two circular sections
   // ISOSCELES TRAPEZOID, Area Moment of Inertia
   // I(area)FF = h/(144*(a+b)*(16*h^2*a*b+4*h^2*b^2+4*h^2*a^2+3*a^4+6*a^2*b^2+6*a^3*b+6*a*b^3+3*b^4)) (centroidial axis)
   // circular sections, Area Moment of Inertia
   // I(area)FB = rb^4/4*(theta - sin(theta)+2/3*sin(theta)*sin(theta/2)^2), where rb is flipper base radius
   // I(area)FE = re^4/4*(theta - sin(theta)+2/3*sin(theta)*sin(theta/2)^2),requires translation to centroidial axis
   // then translate these using the parallel axis theorem to the flipper rotational axis

   const float etheta = (float)M_PI - (fa+fa); // end radius section angle
   const float btheta = (float)M_PI + (fa+fa);	// base radius section angle
   const float tmp1 = sinf(btheta*0.5f);
   const float tmp2 = sinf(etheta*0.5f);
   const float a = 2.0f*endr*tmp2; 
   const float b = 2.0f*baser*tmp1; // face thickness at end and base radii

   const float baseh = baser*cosf(btheta*0.5f);
   const float endh = endr*cosf(etheta*0.5f);
   const float h = flipr + baseh + endh;

   float Irb_inertia = (baser*baser)*(baser*baser)*0.25f*(btheta - sinf(btheta) + (float)(2.0/3.0)*sinf(btheta)*tmp1*tmp1);//base radius
   Irb_inertia /= baser*baser*(btheta - sinf(btheta)); // divide by area to obtain simple Inertia

   float Ire_inertia = (endr*endr)*(endr*endr)*0.25f*(etheta - sinf(etheta) + (float)(2.0/3.0)*sinf(etheta)*(tmp2*tmp2));//end radius
   Ire_inertia /= endr*endr*(etheta - sinf(etheta)); // divide by area

   // translate to centroidal and then flipper axis.. subtract section radius squared then add (flipper radius + section radius) squared
   const float tmp3 = (float)(4.0/3.0)*endr*(tmp2*tmp2)*tmp2/(etheta-sinf(etheta));
   Ire_inertia = Ire_inertia + ((flipr+tmp3)*(flipr+tmp3)
      -       tmp3 *       tmp3); // double parallel axis

   //flipper body trapizoidal section
   float Ifb_inertia = h/(144.0f*(a+b))*(16.0f*(h*h)*a*b + 4.0f*(h*h)*((b*b)+(a*a)) + 3.0f*(a*a)*(a*a)
      + 6.0f*(a*a)*(b*b) + 6.0f*(a*b)*((b*b)+(a*a)) + 3.0f*(b*b)*(b*b));
   Ifb_inertia /= h*0.5f*(a+b); // divide by area

   const float tmp4 = h*(float)(1.0/3.0)*(a+(a+b))/(a+b);
   Ifb_inertia = Ifb_inertia + tmp4*tmp4; //flipper body translated to flipper axis ...parallel axis

   const float Iff = Irb_inertia + Ifb_inertia + Ire_inertia; //scalar moment of inertia ... multiply by weight next

   m_flipperanim.m_inertia = Iff * mass;  //mass of flipper body

   //m_flipperanim.m_inertia = mass;  //stubbed to mass of flipper body
#endif
}

HitFlipper::~HitFlipper()
{
   //m_pflipper->m_phitflipper = NULL;
}

void HitFlipper::CalcHitRect()
{
   // Allow roundoff
   m_rcHitRect.left = m_flipperanim.m_hitcircleBase.center.x - m_flipperanim.m_flipperradius - m_flipperanim.m_endradius - 0.1f;
   m_rcHitRect.right = m_flipperanim.m_hitcircleBase.center.x + m_flipperanim.m_flipperradius + m_flipperanim.m_endradius + 0.1f;
   m_rcHitRect.top = m_flipperanim.m_hitcircleBase.center.y - m_flipperanim.m_flipperradius - m_flipperanim.m_endradius - 0.1f;
   m_rcHitRect.bottom = m_flipperanim.m_hitcircleBase.center.y + m_flipperanim.m_flipperradius + m_flipperanim.m_endradius + 0.1f;
   m_rcHitRect.zlow = m_flipperanim.m_hitcircleBase.zlow;
   m_rcHitRect.zhigh = m_flipperanim.m_hitcircleBase.zhigh;
}


void FlipperAnimObject::SetObjects(const float angle)
{	
   m_angleCur = angle;
   m_hitcircleEnd.center.x = m_hitcircleBase.center.x + m_flipperradius*sinf(angle); //place end radius center
   m_hitcircleEnd.center.y = m_hitcircleBase.center.y - m_flipperradius*cosf(angle);
   m_hitcircleEnd.radius = m_endradius;

   m_lineseg1.normal.x =  sinf(angle - faceNormOffset); // normals to new face positions
   m_lineseg1.normal.y = -cosf(angle - faceNormOffset);
   m_lineseg2.normal.x =  sinf(angle + faceNormOffset);
   m_lineseg2.normal.y = -cosf(angle + faceNormOffset);

   m_lineseg1.v1.x = m_hitcircleEnd.center.x + m_hitcircleEnd.radius*m_lineseg1.normal.x; //new endpoints
   m_lineseg1.v1.y = m_hitcircleEnd.center.y + m_hitcircleEnd.radius*m_lineseg1.normal.y;

   m_lineseg1.v2.x = m_hitcircleBase.center.x + m_hitcircleBase.radius*m_lineseg1.normal.x;
   m_lineseg1.v2.y = m_hitcircleBase.center.y + m_hitcircleBase.radius*m_lineseg1.normal.y;

   m_lineseg2.v1.x = m_hitcircleBase.center.x + m_hitcircleBase.radius*m_lineseg2.normal.x; // remember v1 to v2 direction
   m_lineseg2.v1.y = m_hitcircleBase.center.y + m_hitcircleBase.radius*m_lineseg2.normal.y; // to make sure norm is correct

   m_lineseg2.v2.x = m_hitcircleEnd.center.x + m_hitcircleEnd.radius*m_lineseg2.normal.x;
   m_lineseg2.v2.y = m_hitcircleEnd.center.y + m_hitcircleEnd.radius*m_lineseg2.normal.y;
}


void FlipperAnimObject::UpdateDisplacements(const float dtime)
{
   m_angleCur += m_anglespeed*dtime;	// move flipper angle

   if (m_angleCur > m_angleMax)		// too far???
   {
      m_angleCur = m_angleMax; 

      if (m_anglespeed > 0.f) 
      {
         if(m_fAcc > 0) m_fAcc = 0;

         const float anglespd = fabsf(RADTOANG(m_anglespeed));
         m_anglespeed = 0.f;

         if (m_EnableRotateEvent > 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_EOS,anglespd); // send EOS event
         else if (m_EnableRotateEvent < 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_BOS, anglespd);	// send Beginning of Stroke event
         m_EnableRotateEvent = 0;
      }
   }	
   else if (m_angleCur < m_angleMin)
   {
      m_angleCur = m_angleMin; 

      if (m_anglespeed < 0.f)
      {
         if(m_fAcc < 0) m_fAcc = 0;

         const float anglespd = fabsf(RADTOANG(m_anglespeed));
         m_anglespeed = 0.f;			

         if (m_EnableRotateEvent > 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_EOS,anglespd); // send EOS event
         else if (m_EnableRotateEvent < 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_BOS, anglespd);	// send Park event
         m_EnableRotateEvent = 0;
      }
   }
}

void FlipperAnimObject::UpdateVelocities()
{
   if (m_fAcc == 0)
   {
       //m_anglespeed = 0; //idle
   }
   else if (m_fAcc > 0) // positive ... increasing angle
   {
      m_anglespeed += (m_force/m_mass) * C_FLIPPERACCEL; //new angular rate

      if (m_anglespeed > m_maxvelocity) 
         m_anglespeed = m_maxvelocity; //limit
   }
   else // negative ... decreasing angle
   {
      m_anglespeed -= (m_force/m_mass) * C_FLIPPERACCEL; //new angular rate

      if (m_anglespeed < -m_maxvelocity) 
         m_anglespeed = -m_maxvelocity; //limit
   }
}

#define LeftFace 1
#define RightFace 0

float HitFlipper::HitTest(const Ball * pball, float dtime, CollisionEvent& coll)
{
   if (!m_flipperanim.m_fEnabled) return -1;

   const bool lastface = m_flipperanim.m_lastHitFace;

   //m_flipperanim.SetObjects(m_flipperanim.m_angleCur);	// set current positions ... not needed

   // for effective computing, adding a last face hit value to speed calculations 
   //  a ball can only hit one face never two
   // also if a ball hits a face then it can not hit either radius
   // so only check these if a face is not hit
   // endRadius is more likely than baseRadius ... so check it first

   float hittime = HitTestFlipperFace(pball, dtime, coll, lastface); // first face
   if (hittime >= 0) return hittime;		

   hittime = HitTestFlipperFace(pball, dtime, coll, !lastface); //second face
   if (hittime >= 0)
   {
      m_flipperanim.m_lastHitFace = !lastface;	// change this face to check first
      return hittime;
   }

   hittime = HitTestFlipperEnd(pball, dtime, coll); // end radius
   if (hittime >= 0)
      return hittime;

   hittime = m_flipperanim.m_hitcircleBase.HitTest(pball, dtime, coll);
   if (hittime >= 0)
   {		
      coll.normal[1].x = 0;			//Tangent velocity of contact point (rotate Normal right)
      coll.normal[1].y = 0;			//units: rad*d/t (Radians*diameter/time

      coll.normal[2].x = 0;			//moment is zero ... only friction
      coll.normal[2].y = 0;			//radians/time at collison

      return hittime;
   }

   return -1.0f;	// no hits
}

float HitFlipper::HitTestFlipperEnd(const Ball * pball, const float dtime, CollisionEvent& coll) // replacement
{ 	 
   const float angleCur = m_flipperanim.m_angleCur;
   float anglespeed = m_flipperanim.m_anglespeed;		// rotation rate

   const Vertex2D flipperbase = m_flipperanim.m_hitcircleBase.center;

   const float angleMin = m_flipperanim.m_angleMin;
   const float angleMax = m_flipperanim.m_angleMax;

   const float ballr = pball->radius;
   const float feRadius = m_flipperanim.m_hitcircleEnd.radius;

   const float ballrEndr = feRadius + ballr;			// magnititude of (ball - flipperEnd)

   const float ballx = pball->pos.x;
   const float bally = pball->pos.y;

   const float ballvx = pball->vel.x;
   const float ballvy = pball->vel.y;

   const Vertex2D vp(0.0f,                            //m_flipperradius*sin(0));
      -m_flipperanim.m_flipperradius); //m_flipperradius*(-cos(0));

   float ballvtx, ballvty;	// new ball position at time t in flipper face coordinate
   float contactAng;
   float bfend, cbcedist;
   float t0,t1, d0,d1,dp; // Modified False Position control

   float t = 0; //start first interval ++++++++++++++++++++++++++
   int k;
   for (k=1;k <= C_INTERATIONS;++k)
   {
      // determine flipper rotation direction, limits and parking 

      contactAng = angleCur + anglespeed * t;					// angle at time t

      if (contactAng >= angleMax) contactAng = angleMax;		// stop here			
      else if (contactAng <= angleMin) contactAng = angleMin;	// stop here 

      const float radsin = sinf(contactAng);// Green's transform matrix... rotate angle delta 
      const float radcos = cosf(contactAng);// rotational transform from zero position to position at time t

      //rotate angle delta unit vector, rotates system according to flipper face angle
      const Vertex2D vt(
         vp.x *radcos - vp.y *radsin + flipperbase.x,		//rotate and translate to world position
         vp.y *radcos + vp.x *radsin + flipperbase.y);

      ballvtx = ballx + ballvx*t - vt.x;						// new ball position relative to flipper end radius
      ballvty = bally + ballvy*t - vt.y;

      cbcedist = sqrtf(ballvtx*ballvtx + ballvty*ballvty);	// center ball to center end radius distance

      bfend = cbcedist - ballrEndr;							// ball face-to-radius surface distance

      if (fabsf(bfend) <= C_PRECISION) break; 

      if (k == 1)   // end of pass one ... set full interval pass, t = dtime
      { // test for extreme conditions
         if (bfend < -((float)PHYS_SKIN + feRadius)) return -1.0f;	// too deeply embedded, ambigious position
         if (bfend <= (float)PHYS_TOUCH) 
            break; // inside the clearance limits

         t0 = t1 = dtime; d0 = 0; d1 = bfend; // set for second pass, force t=dtime
      }
      else if (k == 2) // end pass two, check if zero crossing on initial interval, exit if none
      {
         if (dp*bfend > 0.0f) return -1.0f;	// no solution ... no obvious zero crossing

         t0 = 0; t1 = dtime; d0 = dp; d1 = bfend; // set initial boundaries
      }
      else // (k >= 3) // MFP root search +++++++++++++++++++++++++++++++++++++++++
      {
         if (bfend*d0 <= 0.0f)										// zero crossing
         { t1 = t; d1 = bfend; if (dp*bfend > 0.0) d0 *= 0.5f; } // 	move right interval limit			
         else 
         { t0 = t; d0 = bfend; if (dp*bfend > 0.0) d1 *= 0.5f; }	// 	move left interval limit		
      }		

      t = t0 - d0*(t1 - t0)/(d1 - d0);			// estimate next t
      dp = bfend;									// remember 

   }//for loop
   //+++ End time interation loop found time t soultion ++++++

   if (t < 0 || t > dtime							// time is outside this frame ... no collision
      ||
      ((k > C_INTERATIONS) && (fabsf(bfend) > (float)(PHYS_SKIN/4.0)))) // last ditch effort to accept a solution
      return -1.0f; // no solution

   // here ball and flipper end are in contact .. well in most cases, near and embedded solutions need calculations	

   const float hitz = pball->pos.z - ballr + pball->vel.z*t;	// check for a hole, relative to ball rolling point at hittime

   if ((hitz + (ballr * 1.5f)) < m_rcHitRect.zlow		//check limits of object's height and depth
      || (hitz + (ballr * 0.5f)) > m_rcHitRect.zhigh)
      return -1.0f;

   // ok we have a confirmed contact, calc the stats, remember there are "near" solution, so all
   // parameters need to be calculated from the actual configuration, i.e contact radius must be calc'ed

   const float inv_cbcedist = 1.0f/cbcedist;
   coll.normal[0].x = ballvtx*inv_cbcedist;				// normal vector from flipper end to ball
   coll.normal[0].y = ballvty*inv_cbcedist;

   const Vertex2D dist(
      (pball->pos.x + ballvx*t - ballr*coll.normal[0].x - m_flipperanim.m_hitcircleBase.center.x), // vector from base to flipperEnd plus the projected End radius
      (pball->pos.y + ballvy*t - ballr*coll.normal[0].y - m_flipperanim.m_hitcircleBase.center.y));

   const float distance = sqrtf(dist.x*dist.x + dist.y*dist.y);	// distance from base center to contact point

   if ((contactAng >= angleMax && anglespeed > 0) || (contactAng <= angleMin && anglespeed < 0))	// hit limits ??? 
      anglespeed = 0;							// rotation stopped

   const float inv_distance = 1.0f/distance;
   coll.normal[1].x = -dist.y*inv_distance; //Unit Tangent vector velocity of contact point(rotate normal right)
   coll.normal[1].y =  dist.x*inv_distance;

   coll.normal[2].x = distance;				//moment arm diameter
   coll.normal[2].y = anglespeed;			//radians/time at collison

   //recheck using actual contact angle of velocity direction
   const Vertex2D dv(
      (ballvx - coll.normal[1].x *anglespeed*distance), 
      (ballvy - coll.normal[1].y *anglespeed*distance)); //delta velocity ball to face

   const float bnv = dv.x*coll.normal[0].x + dv.y*coll.normal[0].y;  //dot Normal to delta v

   if (bnv >= 0) 
      return -1.0f; // not hit ... ball is receding from face already, must have been embedded or shallow angled

   coll.distance = bfend;				//actual contact distance ..
   coll.hitRigid = true;				// collision type

   return t;
}


float HitFlipper::HitTestFlipperFace(const Ball * pball, const float dtime, CollisionEvent& coll, const bool face)
{ 
   const float angleCur = m_flipperanim.m_angleCur;
   float anglespeed = m_flipperanim.m_anglespeed;				// rotation rate

   const Vertex2D flipperbase = m_flipperanim.m_hitcircleBase.center;
   const float feRadius = m_flipperanim.m_hitcircleEnd.radius;

   const float angleMin = m_flipperanim.m_angleMin;
   const float angleMax = m_flipperanim.m_angleMax;	

   const float ballr = pball->radius;	
   const float ballvx = pball->vel.x;
   const float ballvy = pball->vel.y;	

   // flipper positions at zero degrees rotation

   float ffnx = m_flipperanim.zeroAngNorm.x; // flipper face normal vector //Face2 
   if (face == LeftFace) ffnx = -ffnx;		  // negative for face1

   const float ffny = m_flipperanim.zeroAngNorm.y;	  // norm y component same for either face
   const Vertex2D vp(									  // face segment V1 point
      m_flipperanim.m_hitcircleBase.radius*ffnx, // face endpoint of line segment on base radius
      m_flipperanim.m_hitcircleBase.radius*ffny);		

   Vertex2D F;			// flipper face normal

   float bffnd;		// ball flipper face normal distance (negative for normal side)
   float ballvtx, ballvty;		// new ball position at time t in flipper face coordinate
   float contactAng;

   float t,t0,t1, d0,d1,dp; // Modified False Position control

   t = 0; //start first interval ++++++++++++++++++++++++++
   int k;
   for (k=1; k<=C_INTERATIONS; ++k)
   {
      // determine flipper rotation direction, limits and parking 	

      contactAng = angleCur + anglespeed * t;					// angle at time t

      if (contactAng >= angleMax) contactAng = angleMax;			// stop here			
      else if (contactAng <= angleMin) contactAng = angleMin;		// stop here 

      const float radsin = sinf(contactAng);//  Green's transform matrix... rotate angle delta 
      const float radcos = cosf(contactAng);//  rotational transform from current position to position at time t

      F.x = ffnx *radcos - ffny *radsin;  // rotate to time t, norm and face offset point
      F.y = ffny *radcos + ffnx *radsin;  // 

      const Vertex2D vt(
         vp.x *radcos - vp.y *radsin + flipperbase.x, //rotate and translate to world position
         vp.y *radcos + vp.x *radsin + flipperbase.y);

      ballvtx = pball->pos.x + ballvx*t - vt.x;	// new ball position relative to rotated line segment endpoint
      ballvty = pball->pos.y + ballvy*t - vt.y;	

      bffnd = ballvtx *F.x +  ballvty *F.y - ballr; // normal distance to segment 

      if (fabsf(bffnd) <= C_PRECISION) break;

      // loop control, boundary checks, next estimate, etc.

      if (k == 1)   // end of pass one ... set full interval pass, t = dtime
      {    // test for already inside flipper plane, either embedded or beyond the face endpoints
         if (bffnd < -((float)PHYS_SKIN + feRadius)) return -1.0f;		// wrong side of face, or too deeply embedded			
         if (bffnd <= (float)PHYS_TOUCH) 
            break; // inside the clearance limits, go check face endpoints

         t0 = t1 = dtime; d0 = 0; d1 = bffnd; // set for second pass, so t=dtime
      }
      else if (k == 2)// end pass two, check if zero crossing on initial interval, exit
      {	
         if (dp*bffnd > 0.0) return -1.0f;	// no solution ... no obvious zero crossing
         t0 = 0; t1 = dtime; d0 = dp; d1 = bffnd; // testing MFP estimates			
      }
      else // (k >= 3) // MFP root search +++++++++++++++++++++++++++++++++++++++++
      {
         if (bffnd*d0 <= 0.0)									// zero crossing
         { t1 = t; d1 = bffnd; if (dp*bffnd > 0.0) d0 *= 0.5f; } // 	move right limits
         else 
         { t0 = t; d0 = bffnd; if (dp*bffnd > 0.0) d1 *= 0.5f; } // move left limits
      }		

      t = t0 - d0*(t1-t0)/(d1-d0);					// next estimate
      dp = bffnd;	// remember 
   }//for loop

   //+++ End time interation loop found time t soultion ++++++

   if (t < 0 || t > dtime								// time is outside this frame ... no collision
      ||
      ((k > C_INTERATIONS) && (fabsf(bffnd) > (float)(PHYS_SKIN/4.0)))) // last ditch effort to accept a near solution
      return -1.0f; // no solution

   // here ball and flipper face are in contact... past the endpoints, also, don't forget embedded and near soultion

   Vertex2D T;			// flipper face tangent
   if (face == LeftFace) 
   { T.x = -F.y; T.y = F.x; }	// rotate to form Tangent vector				
   else
   { T.x = F.y; T.y = -F.x; }	// rotate to form Tangent vector

   const float bfftd = ballvtx * T.x + ballvty * T.y;			// ball to flipper face tanget distance	

   const float len = m_flipperanim.m_lineseg1.length;// face segment length ... i.g same on either face									
   if (bfftd < -C_TOL_ENDPNTS || bfftd > len + C_TOL_ENDPNTS) return -1.0f;	// not in range of touching

   const float hitz = pball->pos.z - ballr + pball->vel.z*t;	// check for a hole, relative to ball rolling point at hittime

   if ((hitz + (ballr * 1.5f)) < m_rcHitRect.zlow			//check limits of object's height and depth 
      || (hitz + (ballr * 0.5f)) > m_rcHitRect.zhigh)
      return -1.0f;

   // ok we have a confirmed contact, calc the stats, remember there are "near" solution, so all
   // parameters need to be calculated from the actual configuration, i.e contact radius must be calc'ed

   coll.normal[0].x = F.x;	// hit normal is same as line segment normal
   coll.normal[0].y = F.y;

   const Vertex2D dist( // calculate moment from flipper base center
      (pball->pos.x + ballvx*t - ballr*F.x - m_flipperanim.m_hitcircleBase.center.x),  //center of ball + projected radius to contact point
      (pball->pos.y + ballvy*t - ballr*F.y - m_flipperanim.m_hitcircleBase.center.y)); // all at time t

   const float distance = sqrtf(dist.x*dist.x + dist.y*dist.y);	// distance from base center to contact point

   const float inv_dist = 1.0f/distance;
   coll.normal[1].x = -dist.y*inv_dist;		//Unit Tangent velocity of contact point(rotate Normal clockwise)
   coll.normal[1].y =  dist.x*inv_dist;

   if (contactAng >= angleMax && anglespeed > 0 || contactAng <= angleMin && anglespeed < 0)	// hit limits ??? 
      anglespeed = 0.0f;							// rotation stopped

   coll.normal[2].x = distance;				//moment arm diameter
   coll.normal[2].y = anglespeed;			//radians/time at collison

   const Vertex2D dv(
      (ballvx - coll.normal[1].x *anglespeed*distance), 
      (ballvy - coll.normal[1].y *anglespeed*distance)); //delta velocity ball to face

   const float bnv = dv.x*coll.normal[0].x + dv.y*coll.normal[0].y;  //dot Normal to delta v

   if (bnv >= C_LOWNORMVEL) 
      return -1.0f; // not hit ... ball is receding from endradius already, must have been embedded

   coll.distance = bffnd;				//normal ...actual contact distance ... 
   coll.hitRigid = true;				// collision type

   return t;
}


void HitFlipper::Collide(CollisionEvent *coll)
{
    Ball *pball = coll->ball;
    Vertex3Ds *phitnormal = coll->normal;

   const float distance = phitnormal[2].x;				// moment .... and the flipper response
   const float angsp = m_flipperanim.m_anglespeed;		// angular rate of flipper at impact moment
   float tanspd = distance * angsp;					// distance * anglespeed
   float flipperHit = 0;

   Vertex2D dv(
      pball->vel.x - phitnormal[1].x*tanspd,
      pball->vel.y - phitnormal[1].y*tanspd);					 //delta velocity ball to face

   float bnv = dv.x*phitnormal[0].x + dv.y*phitnormal[0].y; //dot Normal to delta v

   if (bnv >= -C_LOWNORMVEL )							 // nearly receding ... make sure of conditions
   {												 // otherwise if clearly approaching .. process the collision
      if (bnv > C_LOWNORMVEL) return;					 //is this velocity clearly receding (i.e must > a minimum)		
#ifdef C_EMBEDDED
      if (coll->distance < -C_EMBEDDED)
         bnv = -C_EMBEDSHOT;							 // has ball become embedded???, give it a kick
      else return;
#endif		
   }

#ifdef C_DISP_GAIN 
   // correct displacements, mostly from low velocity blindness, an alternative to true acceleration processing
   float hdist = -C_DISP_GAIN * coll->distance;				// distance found in hit detection
   if (hdist > 1.0e-4f)
   {
      if (hdist > C_DISP_LIMIT) 
         hdist = C_DISP_LIMIT;	// crossing ramps, delta noise
      pball->pos.x += hdist * phitnormal->x;	// push along norm, back to free area
      pball->pos.y += hdist * phitnormal->y;	// use the norm, but is not correct
   }
#endif		

   float impulse = 1.005f + m_elasticity;		// hit on static, immovable flipper ... i.e on the stops
   float obliquecorr = 0.0f;

   if ((bnv < -0.25f) && (g_pplayer->m_time_msec - m_last_hittime) > 250) // limit rate to 333 milliseconds per event //!! WTF?
   {
      flipperHit = (distance == 0.0f) ? -1.0f : -bnv; // move event processing to end of collision handler...
   }

   m_last_hittime = g_pplayer->m_time_msec; // keep resetting until idle for 250 milliseconds

   if (distance > 0.f)	// recoil possible 
   {			
      const float maxradius = m_pflipper->m_d.m_FlipperRadius + m_pflipper->m_d.m_EndRadius; 		
      const float recoil = (m_pflipper->m_d.m_OverridePhysics ? m_pflipper->m_d.m_OverrideRecoil : m_pflipper->m_d.m_recoil)/maxradius; // convert to Radians/time
      const float tfdr = distance/maxradius; 		
      const float tfr = powf(tfdr, (m_pflipper->m_d.m_OverridePhysics ? m_pflipper->m_d.m_OverridePowerLaw : m_pflipper->m_d.m_powerlaw));				// apply powerlaw weighting
      const float dvt = dv.x * phitnormal[1].x + dv.y  * phitnormal[1].y;		// velocity transvere to flipper
      const float anglespeed = m_flipperanim.m_anglespeed + dvt * tfr * impulse/(distance*(m_forcemass + tfr));		

      if (m_flipperanim.m_fAcc != 0)											// currently in rotation
      {	
         obliquecorr = (float)m_flipperanim.m_fAcc * (m_pflipper->m_d.m_OverridePhysics ? m_pflipper->m_d.m_OverrideOblique : m_pflipper->m_d.m_obliquecorrection); //flipper trajectory correction
         impulse = (1.005f + m_elasticity)*m_forcemass/(m_forcemass + tfr);	// impulse for pinball
         m_flipperanim.m_anglespeed = anglespeed;							// new angle speed for flipper	
      }
      else if (recoil > 0.f && fabsf(anglespeed) > recoil)					// discard small static impact motions
      { // these effects are for the flipper at EOS (End of Stroke)
         if (anglespeed < 0.f && m_flipperanim.m_angleCur >= m_flipperanim.m_angleMax) // at max angle now?
         { // rotation toward minimum angle					
            m_flipperanim.m_force = max(-(anglespeed+anglespeed),0.005f);	// restoring force
            impulse = (1.005f + m_elasticity)*m_forcemass/(m_forcemass + tfr); // impulse for pinball
            m_flipperanim.m_anglespeed = anglespeed;						// angle speed, less linkage losses, etc.
            m_flipperanim.m_fAcc = 1;										// set acceleration to opposite direction
            if (anglespeed < -0.05f)
               m_flipperanim.m_EnableRotateEvent = 1; //make EOS event
         }
         else if (anglespeed > 0.f && m_flipperanim.m_angleCur <= m_flipperanim.m_angleMin) // at min angle now?
         {// rotation toward maximum angle
            m_flipperanim.m_force = max(anglespeed+anglespeed,0.005f);		// restoreing force
            impulse = (1.005f + m_elasticity)*m_forcemass/(m_forcemass + tfr); // impulse for pinball
            m_flipperanim.m_anglespeed = anglespeed;						// angle speed, less linkage losses, etc.
            m_flipperanim.m_fAcc = -1;										// set acceleration to opposite direction
            if (anglespeed > 0.05f) 
               m_flipperanim.m_EnableRotateEvent = 1; //make EOS event
         }
      }	
   }
   pball->vel.x -= impulse*bnv * phitnormal->x; 							// new velocity for ball after impact
   pball->vel.y -= impulse*bnv * phitnormal->y;

   float scatter_angle = (m_pflipper->m_d.m_OverridePhysics ? m_pflipper->m_d.m_OverrideScatter : m_pflipper->m_d.m_scatterangle); // object specific roughness
   if (scatter_angle <= 0.0f)
      scatter_angle = c_hardScatter;
   scatter_angle *= g_pplayer->m_ptable->m_globalDifficulty;			// apply difficulty weighting

   if (bnv > -1.0f) scatter_angle = 0.f;								// not for low velocities

   if (obliquecorr != 0.f || scatter_angle > 1.0e-5f)					// trajectory correction to reduce the obliqueness 
   {
      float scatter = rand_mt_m11();			    // -1.0f..1.0f
      scatter *= (1.0f - scatter*scatter) * 2.59808f * scatter_angle;	// shape quadratic distribution and scale
      scatter_angle = obliquecorr + scatter;
      const float radsin = sinf(scatter_angle);	//  Green's transform matrix... rotate angle delta 
      const float radcos = cosf(scatter_angle);	//  rotational transform from current position to position at time t
      const float vx2 = pball->vel.x;
      const float vy2 = pball->vel.y;
      pball->vel.x = vx2 *radcos - vy2 *radsin;  // rotate trajectory more accurately
      pball->vel.y = vy2 *radcos + vx2 *radsin;
   }

   pball->vel.x *= 0.985f; pball->vel.y *= 0.985f; pball->vel.z *= 0.96f;	// friction

   pball->m_fDynamic = C_DYNAMIC;			// reactive ball if quenched

   tanspd = m_flipperanim.m_anglespeed *distance; // new tangential speed
   dv.x = (pball->vel.x - phitnormal[1].x * tanspd); // project along unit transverse vector
   dv.y = (pball->vel.y - phitnormal[1].y * tanspd); // delta velocity

   bnv = dv.x*phitnormal->x + dv.y*phitnormal->y;	// dot face Normal to delta v

   if (bnv < 0)
   {	// opps .... impulse calculations were off a bit, add a little boost
      bnv *= -1.2f;						// small bounce
      pball->vel.x += bnv * phitnormal->x;	// new velocity for ball after corrected impact
      pball->vel.y += bnv * phitnormal->y;	//
   }

   // move hit event to end of collision routine, pinball may be deleted
   if (flipperHit != 0)
   {
      if (flipperHit < 0) m_pflipper->FireGroupEvent(DISPID_HitEvents_Hit);	   //simple hit event	
      else m_pflipper->FireVoidEventParm(DISPID_FlipperEvents_Collide,flipperHit); // collision velocity (normal to face)	
   }

   const Vertex3Ds vnormal(phitnormal->x, phitnormal->y, 0.0f);
   pball->AngularAcceleration(vnormal);
}

