#include "stdafx.h"

HitFlipper::HitFlipper(const Vertex2D& center, float baser, float endr, float flipr, float angleStart, float angleEnd,
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

   m_flipperanim.m_hitcircleBase.center = center;

   if (baser < 0.01f) baser = 0.01f; // must not be zero 
   m_flipperanim.m_hitcircleBase.radius = baser; //radius of base section

   if (endr < 0.01f) endr = 0.01f; // must not be zero 
   m_flipperanim.m_endradius = endr;		// radius of flipper end

   if (flipr < 0.01f) flipr = 0.01f; // must not be zero 
   m_flipperanim.m_flipperradius = flipr;	//radius of flipper arc, center-to-center radius

   m_flipperanim.m_dir = (angleEnd >= angleStart) ? 1 : -1;
   m_flipperanim.m_solState = false;

   m_flipperanim.m_angleStart = angleStart;
   m_flipperanim.m_angleEnd   = angleEnd;
   m_flipperanim.m_angleMin   = min(angleStart, angleEnd);
   m_flipperanim.m_angleMax   = max(angleStart, angleEnd);
   m_flipperanim.m_angleCur   = angleStart;

   m_flipperanim.m_angularMomentum = 0;
   m_flipperanim.m_angularAcceleration = 0;
   m_flipperanim.m_anglespeed = 0;

   const float fa = asinf((baser-endr)/flipr); //face to centerline angle (center to center)

   m_flipperanim.faceNormOffset = (float)(M_PI/2.0) - fa; //angle of normal when flipper center line at angle zero

   m_flipperanim.SetObjects(angleStart);

   m_flipperanim.m_force = strength;
   m_flipperanim.m_mass = mass;

   // model inertia of flipper as that of rod of length flipr around its end
   m_flipperanim.m_inertia = 1.0f/3.0f * mass * (flipr*flipr);

   m_last_hittime = 0;

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

   //if (m_anglespeed)
   //    slintf("Ang.speed: %f\n", m_anglespeed);

   if (m_angleCur >= m_angleMax)        // hit stop?
   {
      //m_angleCur = m_angleMax;

      if (m_anglespeed > 0.f) 
      {
#ifdef DEBUG_FLIPPERS
         if (m_startTime)
         {
             U32 dur = g_pplayer->m_time_msec - m_startTime;
             m_startTime = 0;
             slintf("Stroke duration: %u ms\nAng. velocity: %f\n", dur, m_anglespeed);
             slintf("Ball velocity: %f\n", g_pplayer->m_vball[0]->vel.Length());
         }
#endif

         const float anglespd = fabsf(RADTOANG(m_anglespeed));
         m_angularMomentum *= -0.2f;
         m_anglespeed = m_angularMomentum / m_inertia;

         if (m_EnableRotateEvent > 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_EOS,anglespd); // send EOS event
         else if (m_EnableRotateEvent < 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_BOS, anglespd);	// send Beginning of Stroke event
         m_EnableRotateEvent = 0;
      }
   }
   else if (m_angleCur <= m_angleMin)
   {
      //m_angleCur = m_angleMin;

      if (m_anglespeed < 0.f)
      {
         const float anglespd = fabsf(RADTOANG(m_anglespeed));
         m_angularMomentum *= -0.2f;
         m_anglespeed = m_angularMomentum / m_inertia;

         if (m_EnableRotateEvent > 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_EOS,anglespd); // send EOS event
         else if (m_EnableRotateEvent < 0) m_pflipper->FireVoidEventParm(DISPID_LimitEvents_BOS, anglespd);	// send Park event
         m_EnableRotateEvent = 0;
      }
   }
}

void FlipperAnimObject::UpdateVelocities()
{
    //const float springDispl = GetStrokeRatio() * 0.5f + 0.5f; // range: [0.5 .. 1]
    //const float springForce = -0.6f * springDispl * m_force;
    //const float solForce = m_solState ? m_force : 0.0f;
    //float force = m_dir * (solForce + springForce);

    float solTorque = 0;
    if (m_solState)
    {
        solTorque = m_force;
        if (fabsf(m_angleCur - m_angleEnd) <= M_PI/180.0f)
            solTorque *= 0.33f;     // hold coil is weaker
    }

    float torque = m_dir * (m_solState ? solTorque : -0.1f * m_force);

    // resolve contacts with stoppers
    if (fabsf(m_anglespeed) <= 1e-2f)
    {
        if ((m_angleCur >= m_angleMax - 1e-2f && torque > 0)
         || (m_angleCur <= m_angleMin + 1e-2f && torque < 0))
        {
            m_angularMomentum = 0;
            torque = 0;
        }
    }

    m_angularMomentum += PHYS_FACTOR * torque;
    m_anglespeed = m_angularMomentum / m_inertia;
    m_angularAcceleration = torque / m_inertia;
}

void FlipperAnimObject::ApplyImpulse(const Vertex3Ds& surfP, const Vertex3Ds& impulse)
{
    const Vertex3Ds rotI = CrossProduct(surfP, impulse);
    m_angularMomentum += rotI.z;            // only rotation about z axis
    m_anglespeed = m_angularMomentum / m_inertia;    // TODO: figure out moment of inertia
}


void FlipperAnimObject::SetSolenoidState(bool s)
{
    m_solState = s;
#ifdef DEBUG_FLIPPERS
    if (m_angleCur == m_angleStart)
        m_startTime = g_pplayer->m_time_msec;
#endif
}

float FlipperAnimObject::GetStrokeRatio() const
{
    return (m_angleCur - m_angleStart) / (m_angleEnd - m_angleStart);
}

// compute the cross product (0,0,rz) x v
static inline Vertex3Ds CrossZ(float rz, const Vertex3Ds& v)
{
    return Vertex3Ds( -rz * v.y, rz * v.x, 0 );
}

Vertex3Ds FlipperAnimObject::SurfaceVelocity(const Vertex3Ds& surfP) const
{
    //const Vertex3Ds angularVelocity(0, 0, m_anglespeed);
    //return CrossProduct( angularVelocity, surfP );
    // equivalent:
    return CrossZ(m_anglespeed, surfP);
}

Vertex3Ds FlipperAnimObject::SurfaceAcceleration(const Vertex3Ds& surfP) const
{
    // tangential acceleration = (0, 0, omega) x surfP
    const Vertex3Ds tangAcc = CrossZ(m_angularAcceleration, surfP);

    // centripetal acceleration = (0,0,omega) x ( (0,0,omega) x surfP )
    const float av2 = m_anglespeed * m_anglespeed;
    const Vertex3Ds centrAcc( -av2 * surfP.x, -av2 * surfP.y, 0 );

    return tangAcc + centrAcc;
}

float FlipperAnimObject::GetHitTime() const
{
    if (m_anglespeed == 0)
        return -1.0f;

    const float dist = (m_anglespeed > 0)
        ? m_angleMax - m_angleCur       // >= 0
        : m_angleMin - m_angleCur;      // <= 0

    const float hittime = dist / m_anglespeed;

    if (infNaN(hittime) || hittime < 0)
        return -1.0f;
    else
        return hittime;
}

float HitFlipper::GetHitTime() const
{
    return m_flipperanim.GetHitTime();
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
   coll.normal[0].z = 0.0f;

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

   if (fabsf(bnv) <= C_CONTACTVEL && bfend <= PHYS_TOUCH)
   {
       coll.isContact = true;
       coll.normal[3].z = bnv;
   }
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
   coll.normal[0].z = 0.0f;

   const Vertex2D dist( // calculate moment from flipper base center
      (pball->pos.x + ballvx*t - ballr*F.x - m_flipperanim.m_hitcircleBase.center.x),  //center of ball + projected radius to contact point
      (pball->pos.y + ballvy*t - ballr*F.y - m_flipperanim.m_hitcircleBase.center.y)); // all at time t

   const float distance = sqrtf(dist.x*dist.x + dist.y*dist.y);	// distance from base center to contact point

   const float inv_dist = 1.0f/distance;
   coll.normal[1].x = -dist.y*inv_dist;		//Unit Tangent velocity of contact point(rotate Normal clockwise)
   coll.normal[1].y =  dist.x*inv_dist;
   coll.normal[1].z = 0.0f;

   if (contactAng >= angleMax && anglespeed > 0 || contactAng <= angleMin && anglespeed < 0)	// hit limits ??? 
      anglespeed = 0.0f;							// rotation stopped

   coll.normal[2].x = distance;				//moment arm diameter
   coll.normal[2].y = anglespeed;			//radians/time at collison

   const Vertex2D dv(
      (ballvx - coll.normal[1].x *anglespeed*distance), 
      (ballvy - coll.normal[1].y *anglespeed*distance)); //delta velocity ball to face

   const float bnv = dv.x*coll.normal[0].x + dv.y*coll.normal[0].y;  //dot Normal to delta v

   if (fabsf(bnv) <= C_CONTACTVEL && bffnd <= PHYS_TOUCH)
   {
       coll.isContact = true;
       coll.normal[3].z = bnv;
   }
   else if (bnv > C_LOWNORMVEL)
      return -1.0f; // not hit ... ball is receding from endradius already, must have been embedded

   coll.distance = bffnd;				//normal ...actual contact distance ... 
   coll.hitRigid = true;				// collision type

   return t;
}


void HitFlipper::Collide(CollisionEvent *coll)
{
    Ball *pball = coll->ball;
    Vertex3Ds *phitnormal = coll->normal;
    const Vertex3Ds normal = phitnormal[0];

    const Vertex3Ds rB = -pball->radius * normal;
    const Vertex3Ds hitPos = pball->pos + rB;

    const Vertex3Ds cF(
            m_flipperanim.m_hitcircleBase.center.x,
            m_flipperanim.m_hitcircleBase.center.y,
            pball->pos.z );     // make sure collision happens in same z plane where ball is

    const Vertex3Ds rF = hitPos - cF;       // displacement relative to flipper center

    const Vertex3Ds vB = pball->SurfaceVelocity(rB);
    const Vertex3Ds vF = m_flipperanim.SurfaceVelocity(rF);
    const Vertex3Ds vrel = vB - vF;
    //slintf("Normal: %.2f %.2f %.2f  -  Rel.vel.: %f %f %f\n", normal.x, normal.y, normal.z, vrel.x, vrel.y, vrel.z);

    float bnv = normal.Dot(vrel);       // relative normal velocity
    //slintf("Flipper collision - rel.vel. %f\n", bnv);

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

   const Vertex3Ds tmp = CrossProduct( rF, normal ) / m_flipperanim.m_inertia;        // TODO: proper inertia

   const float impulse = -(1.0f + m_elasticity) * bnv
       / (pball->m_invMass + normal.Dot(CrossProduct(tmp, rF)));

/*
   if (distance > 0.f)	// recoil possible 
   {			
      float obliquecorr = 0.0f;
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
*/

   pball->vel += (impulse * pball->m_invMass) * normal;        // new velocity for ball after impact
   m_flipperanim.ApplyImpulse(rF, -impulse * normal);

   // apply friction

   Vertex3Ds tangent = vrel - vrel.Dot(normal) * normal;       // calc the tangential velocity

   static const float frictionCoeff = 0.6f;

   const float tangentSpSq = tangent.LengthSquared();
   if (tangent.LengthSquared() > 1e-6f)
   {
       tangent /= sqrt(tangentSpSq);            // normalize to get tangent direction
       const float vt = vrel.Dot(tangent);   // get speed in tangential direction

       // compute friction impulse
       Vertex3Ds cross = CrossProduct(rB, tangent);
       float kt = pball->m_invMass + tangent.Dot(CrossProduct(pball->m_inverseworldinertiatensor * cross, rB));

       cross = CrossProduct(rF, tangent);
       kt += tangent.Dot(CrossProduct(cross / m_flipperanim.m_inertia, rF));    // flipper only has angular response

       // friction impulse can't be greater than coefficient of friction times collision impulse (Coulomb friction cone)
       const float maxFric = frictionCoeff * impulse;
       const float jt = clamp(-vt / kt, -maxFric, maxFric);

       pball->ApplySurfaceImpulse(rB, jt * tangent);
       m_flipperanim.ApplyImpulse(rF, -jt * tangent);
   }

   pball->m_fDynamic = C_DYNAMIC;           // reactive ball if quenched

   if ((bnv < -0.25f) && (g_pplayer->m_time_msec - m_last_hittime) > 250) // limit rate to 250 milliseconds per event
   {
       const float distance = phitnormal[2].x;                     // moment .... and the flipper response
       const float flipperHit = (distance == 0.0f) ? -1.0f : -bnv; // move event processing to end of collision handler...
       if (flipperHit < 0)
           m_pflipper->FireGroupEvent(DISPID_HitEvents_Hit);        // simple hit event
       else
           m_pflipper->FireVoidEventParm(DISPID_FlipperEvents_Collide, flipperHit); // collision velocity (normal to face)
   }

   m_last_hittime = g_pplayer->m_time_msec; // keep resetting until idle for 250 milliseconds
}

void HitFlipper::Contact(CollisionEvent& coll, float dtime)
{
    Ball * pball = coll.ball;
    const Vertex3Ds normal = coll.normal[0];
    const float origNormVel = coll.normal[3].z;

    const Vertex3Ds rB = -pball->radius * normal;
    const Vertex3Ds hitPos = pball->pos + rB;

    const Vertex3Ds cF(
            m_flipperanim.m_hitcircleBase.center.x,
            m_flipperanim.m_hitcircleBase.center.y,
            pball->pos.z );     // make sure collision happens in same z plane where ball is

    const Vertex3Ds rF = hitPos - cF;       // displacement relative to flipper center

    const Vertex3Ds vB = pball->SurfaceVelocity(rB);
    const Vertex3Ds vF = m_flipperanim.SurfaceVelocity(rF);
    const Vertex3Ds vrel = vB - vF;

    const float normVel = vrel.Dot(normal);   // this should be zero, but only up to +/- C_CONTACTVEL

    //slintf("Flipper contact - rel.vel. %f\n", normVel);

    // If some collision has changed the ball's velocity, we may not have to do anything.
    if (normVel <= C_CONTACTVEL)
    {
        // compute accelerations of point on ball and flipper
        const Vertex3Ds aB = pball->SurfaceAcceleration(rB);
        const Vertex3Ds aF = m_flipperanim.SurfaceAcceleration(rF);
        const Vertex3Ds arel = aB - aF;

        // time derivative of the normal vector
        const Vertex3Ds normalDeriv = CrossZ(m_flipperanim.m_anglespeed, normal);

        // relative acceleration in the normal direction
        const float normAcc = arel.Dot(normal) + 2 * normalDeriv.Dot(vrel);

        if (normAcc >= 0)
            return;     // objects accelerating away from each other, nothing to do

        // hypothetical accelerations arising from a unit contact force in normal direction
        const Vertex3Ds aBc = pball->m_invMass * normal;
        const Vertex3Ds aFc = CrossProduct( CrossProduct(rF, -normal) / m_flipperanim.m_inertia, rF );       // TODO: inertia
        const float contactForceAcc = normal.Dot( aBc - aFc );

        assert( contactForceAcc > 0 );

        // find j >= 0 such that normAcc + j * contactForceAcc >= 0  (bodies should not accelerate towards each other)

        const float j = -normAcc / contactForceAcc;

        pball->vel += (j * pball->m_invMass * dtime) * normal;
        m_flipperanim.ApplyImpulse(rF, (-j * dtime) * normal);

        //const Vertex3Ds fe = m_mass * g_pplayer->m_gravity;      // external forces (only gravity for now)
        //const float dot = fe.Dot(normal);
        //const float normalForce = std::max( 0.0f, -(dot*dtime + origNormVel) ); // normal force is always nonnegative

        //// Add just enough to kill original normal velocity and counteract the external forces.
        //vel += normalForce * normal;

        //ApplyFriction(normal, dtime, friction);
    }
}
