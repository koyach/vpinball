#include "stdafx.h"
#include "Texture.h"
#include "freeimage.h"

RenderDevice *Texture::renderDevice=0;


MemTexture* MemTexture::CreateFromFreeImage(FIBITMAP* dib)
{
    FIBITMAP* dib32 = FreeImage_ConvertTo32Bits(dib);

    MemTexture* tex = new MemTexture(FreeImage_GetWidth(dib32), FreeImage_GetHeight(dib32));

    BYTE *psrc = FreeImage_GetBits(dib32), *pdst = tex->data();
    int pitchdst = FreeImage_GetPitch(dib32), pitchsrc = tex->pitch();
    const int height = tex->height();

    for (int y = 0; y < height; ++y)
    {
        memcpy(pdst + (height-y-1)*pitchdst, psrc + y*pitchsrc, 4 * tex->width());
    }

    //memcpy(tex->data(), FreeImage_GetBits(dib32), tex->pitch()*tex->height());

    FreeImage_Unload(dib32);
    return tex;
}

BaseTexture* MemTexture::CreateFromFile(const char *szfile)
{
   FREE_IMAGE_FORMAT fif = FIF_UNKNOWN;

   // check the file signature and deduce its format
   // (the second argument is currently not used by FreeImage)
   fif = FreeImage_GetFileType(szfile, 0);
   if(fif == FIF_UNKNOWN) {
      // no signature ?
      // try to guess the file format from the file extension
      fif = FreeImage_GetFIFFromFilename(szfile);
   }
   // check that the plugin has reading capabilities ...
   if((fif != FIF_UNKNOWN) && FreeImage_FIFSupportsReading(fif)) {
      // ok, let's load the file
      FIBITMAP *dib = FreeImage_Load(fif, szfile, 0);

      // check if Textures exceed the maximum texture diemension
      int maxTexDim;
      HRESULT hrMaxTex = GetRegInt("Player", "MaxTexDimension", &maxTexDim);
      if (hrMaxTex != S_OK)
      {
         maxTexDim = 0; // default: Don't resize textures
      }
      if(maxTexDim <= 0)
      {
         maxTexDim = 65536;
      }
      const int pictureWidth = FreeImage_GetWidth(dib);
      const int pictureHeight = FreeImage_GetHeight(dib);
      // save original width and height, if the texture is rescaled
	  if ((pictureHeight > maxTexDim) || (pictureWidth > maxTexDim))
	  {
         int newWidth = min(pictureWidth,maxTexDim);
         int newHeight = min(pictureHeight,maxTexDim);
         if (pictureWidth - newWidth > pictureHeight - newHeight)
             newHeight = min(pictureHeight * newWidth / pictureWidth, maxTexDim);
         else
             newWidth = min(pictureWidth * newHeight / pictureHeight, maxTexDim);
         dib = FreeImage_Rescale(dib, newWidth, newHeight, FILTER_BILINEAR);
      }

      MemTexture* mySurface = MemTexture::CreateFromFreeImage(dib);

      FreeImage_Unload(dib);

      //if (bitsPerPixel == 24)
      //   Texture::SetOpaque(mySurface);

      return mySurface;
   }
   else
      return NULL;
}

// from the FreeImage FAQ page
static FIBITMAP* HBitmapToFreeImage(HBITMAP hbmp)
{
    BITMAP bm;
    GetObject(hbmp, sizeof(BITMAP), &bm);
    FIBITMAP* dib = FreeImage_Allocate(bm.bmWidth, bm.bmHeight, bm.bmBitsPixel);
    // The GetDIBits function clears the biClrUsed and biClrImportant BITMAPINFO members (dont't know why)
    // So we save these infos below. This is needed for palettized images only.
    int nColors = FreeImage_GetColorsUsed(dib);
    HDC dc = GetDC(NULL);
    int Success = GetDIBits(dc, hbmp, 0, FreeImage_GetHeight(dib),
            FreeImage_GetBits(dib), FreeImage_GetInfo(dib), DIB_RGB_COLORS);
    ReleaseDC(NULL, dc);
    // restore BITMAPINFO members
    FreeImage_GetInfoHeader(dib)->biClrUsed = nColors;
    FreeImage_GetInfoHeader(dib)->biClrImportant = nColors;
    return dib;
}

MemTexture* MemTexture::CreateFromHBitmap(HBITMAP hbm)
{
    FIBITMAP *dib = HBitmapToFreeImage(hbm);
    BaseTexture* pdds = MemTexture::CreateFromFreeImage(dib);
    FreeImage_Unload(dib);
    return pdds;
}


////////////////////////////////////////////////////////////////////////////////


Texture::Texture()
{
   m_pdsBuffer = NULL;
   m_pdsBufferColorKey = NULL;
   m_pdsBufferBackdrop = NULL;
   m_rgbTransparent = RGB(255,255,255);
   m_hbmGDIVersion = NULL;
   m_ppb = NULL;
}

Texture::~Texture()
{
   FreeStuff();
}

void Texture::SetRenderDevice( RenderDevice *_device )
{
   renderDevice = _device;
}

void Texture::Release()
{
}

void Texture::Set(DWORD textureChannel)
{
    g_pplayer->m_pin3d.SetBaseTexture( textureChannel, m_pdsBufferColorKey);
}

void Texture::SetBackDrop( DWORD textureChannel )
{
    g_pplayer->m_pin3d.SetBaseTexture( textureChannel, m_pdsBufferBackdrop ? m_pdsBufferBackdrop : NULL);
}

void Texture::Unset( DWORD textureChannel )
{
    g_pplayer->m_pin3d.SetBaseTexture(textureChannel, NULL);
}

HRESULT Texture::SaveToStream(IStream *pstream, PinTable *pt)
{
   BiffWriter bw(pstream, NULL, NULL);

   bw.WriteString(FID(NAME), m_szName);

   bw.WriteString(FID(INME), m_szInternalName);

   bw.WriteString(FID(PATH), m_szPath);

   bw.WriteInt(FID(WDTH), m_originalWidth);
   bw.WriteInt(FID(HGHT), m_originalHeight);

   bw.WriteInt(FID(TRNS), m_rgbTransparent);

   if (!m_ppb)
   {
      bw.WriteTag(FID(BITS));

      // 32-bit picture
      LZWWriter lzwwriter(pstream, (int *)m_pdsBuffer->data(), m_width*4, m_height, m_pdsBuffer->pitch());
      lzwwriter.CompressBits(8+1);
   }
   else // JPEG (or other binary format)
   {
      const int linkid = pt->GetImageLink(this);
      if (linkid == 0)
      {
         bw.WriteTag(FID(JPEG));
         m_ppb->SaveToStream(pstream);
      }
      else
      {
         bw.WriteInt(FID(LINK), linkid);
      }
   }

   bw.WriteTag(FID(ENDB));

   return S_OK;
}

HRESULT Texture::LoadFromStream(IStream *pstream, int version, PinTable *pt)
{
   BiffReader br(pstream, this, pt, version, NULL, NULL);

   br.Load();

   EnsureMaxTextureCoordinates();

   return ((m_pdsBuffer != NULL) ? S_OK : E_FAIL);
}


bool Texture::LoadFromMemory(BYTE *data, DWORD size)
{
    FIMEMORY *hmem = FreeImage_OpenMemory(data, size);
    FREE_IMAGE_FORMAT fif = FreeImage_GetFileTypeFromMemory(hmem, 0);
    FIBITMAP *dib = FreeImage_LoadFromMemory(fif, hmem, 0);

    // check if Textures exceed the maximum texture dimension
    int maxTexDim;
    HRESULT hrMaxTex = GetRegInt("Player", "MaxTexDimension", &maxTexDim);
    if (hrMaxTex != S_OK)
    {
        maxTexDim = 0; // default: Don't resize textures
    }
    if(maxTexDim <= 0)
    {
        maxTexDim = 65536;
    }
    int pictureWidth = FreeImage_GetWidth(dib);
    int pictureHeight = FreeImage_GetHeight(dib);
    // save original width and height, if the texture is rescaled
    m_originalWidth = pictureWidth;
    m_originalHeight = pictureHeight;
    if ((pictureHeight > maxTexDim) || (pictureWidth > maxTexDim))
    {
         int newWidth = min(pictureWidth,maxTexDim);
         int newHeight = min(pictureHeight,maxTexDim);
         if (m_originalWidth - newWidth > m_originalHeight - newHeight)
             newHeight = min(m_originalHeight * newWidth / m_originalWidth, maxTexDim);
         else
             newWidth = min(m_originalWidth * newHeight / m_originalHeight, maxTexDim);
         dib = FreeImage_Rescale(dib, newWidth, newHeight, FILTER_BILINEAR);
         m_width = newWidth;
         m_height = newHeight;
    }

    m_pdsBuffer = MemTexture::CreateFromFreeImage(dib);
    SetSizeFrom(m_pdsBuffer);

    FreeImage_Unload(dib);

    return true;
}


BOOL Texture::LoadToken(int id, BiffReader *pbr)
{
   if (id == FID(NAME))
   {
      pbr->GetString(m_szName);
   }
   else if (id == FID(INME))
   {
      pbr->GetString(m_szInternalName);
   }
   else if (id == FID(PATH))
   {
      pbr->GetString(m_szPath);
   }
   else if (id == FID(TRNS))
   {
      pbr->GetInt(&m_rgbTransparent);
   }
   else if (id == FID(WDTH))
   {
      pbr->GetInt(&m_width);
      m_originalWidth = m_width;
   }
   else if (id == FID(HGHT))
   {
      pbr->GetInt(&m_height);
      m_originalHeight = m_height;
   }
   else if (id == FID(BITS))
   {
       m_pdsBuffer = new MemTexture(m_width, m_height);

      // 32-bit picture
      LZWReader lzwreader(pbr->m_pistream, (int *)m_pdsBuffer->data(), m_width*4, m_height, m_pdsBuffer->pitch());
      lzwreader.Decoder();

      const int lpitch = m_pdsBuffer->pitch();

      // Assume our 32 bit color structure
      // Find out if all alpha values are zero
      BYTE * const pch = (BYTE *)m_pdsBuffer->data();
      bool allAlphaZero = true;
      for (int i=0;i<m_height;i++)
      {
         for (int l=0;l<m_width;l++)
         {
            if (pch[i*lpitch + 4*l + 3] != 0)
            {
                allAlphaZero = false;
                goto endAlphaCheck;
            }
         }
      }
   endAlphaCheck:

      // all alpha values are 0: set them all to 0xff
      if (allAlphaZero)
         for (int i=0;i<m_height;i++)
            for (int l=0;l<m_width;l++)
               pch[i*lpitch + 4*l + 3] = 0xff;
   }
   else if (id == FID(JPEG))
   {
      m_ppb = new PinBinary();
      m_ppb->LoadFromStream(pbr->m_pistream, pbr->m_version);
      // m_ppb->m_szPath has the original filename
      // m_ppb->m_pdata() is the buffer
      // m_ppb->m_cdata() is the filesize
      return LoadFromMemory((BYTE*)m_ppb->m_pdata, m_ppb->m_cdata);
   }
   else if (id == FID(LINK))
   {
      int linkid;
      PinTable * const pt = (PinTable *)pbr->m_pdata;
      pbr->GetInt(&linkid);
      m_ppb = pt->GetImageLinkBinary(linkid);
      return LoadFromMemory((BYTE*)m_ppb->m_pdata, m_ppb->m_cdata);
   }
   return fTrue;
}

void Texture::SetTransparentColor(const COLORREF color)
{
   m_fTransparent = fFalse;
   if (m_rgbTransparent != color)
   {
      m_rgbTransparent = color;
      delete m_pdsBufferColorKey; m_pdsBufferColorKey = NULL;
      delete m_pdsBufferBackdrop; m_pdsBufferBackdrop = NULL;
   }
}

void Texture::CreateAlphaChannel()
{
   if (!m_pdsBufferColorKey)
   {
      // copy buffer into new color key buffer
      m_pdsBufferColorKey = new MemTexture(*m_pdsBuffer);
      m_fTransparent = Texture::SetAlpha(m_pdsBufferColorKey, m_rgbTransparent);
      if (!m_fTransparent)
         m_rgbTransparent = NOTRANSCOLOR; // set to magic color to disable future checking
      CreateNextMipMapLevel(m_pdsBufferColorKey);
   }
}

void Texture::EnsureBackdrop(const COLORREF color)
{
   if (!m_pdsBufferBackdrop || color != m_rgbBackdropCur)
   {
      if (!m_pdsBufferBackdrop)
      {
          m_pdsBufferBackdrop = new MemTexture;
      }
      *m_pdsBufferBackdrop = *m_pdsBuffer;  // copy texture
      SetOpaqueBackdrop(m_pdsBufferBackdrop, m_rgbTransparent, color);
      CreateNextMipMapLevel(m_pdsBufferBackdrop);
      m_rgbBackdropCur = color;
   }
}

void Texture::EnsureMaxTextureCoordinates()
{
   //DWORD texWidth, texHeight;
   //g_pplayer->m_pin3d.m_pd3dDevice->GetTextureSize(m_pdsBuffer, &texWidth, &texHeight);

   //m_maxtu = (float)m_width / (float)texWidth;
   //m_maxtv = (float)m_height / (float)texHeight;
   // TODO (DX9): tex coords
   m_maxtu = m_maxtv = 1.0f;
}

void Texture::FreeStuff()
{
   delete m_pdsBuffer; m_pdsBuffer = NULL;
   delete m_pdsBufferColorKey; m_pdsBufferColorKey = NULL;
   delete m_pdsBufferBackdrop; m_pdsBufferBackdrop = NULL;
   if (m_hbmGDIVersion)
   {
      DeleteObject(m_hbmGDIVersion);
      m_hbmGDIVersion = NULL;
   }
   if (m_ppb)
   {
      delete m_ppb;
      m_ppb = NULL;
   }
}

void Texture::EnsureHBitmap()
{
   if (!m_hbmGDIVersion)
   {
      CreateGDIVersion();
   }
}

void Texture::CreateGDIVersion()
{
   HDC hdcScreen = GetDC(NULL);
   m_hbmGDIVersion = CreateCompatibleBitmap(hdcScreen, m_width, m_height);
   HDC hdcNew = CreateCompatibleDC(hdcScreen);
   HBITMAP hbmOld = (HBITMAP)SelectObject(hdcNew, m_hbmGDIVersion);

   BITMAPINFO bmi;
   ZeroMemory(&bmi, sizeof(bmi));
   bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
   bmi.bmiHeader.biWidth = m_width;
   bmi.bmiHeader.biHeight = -m_height;
   bmi.bmiHeader.biPlanes = 1;
   bmi.bmiHeader.biBitCount = 32;
   bmi.bmiHeader.biCompression = BI_RGB;
   bmi.bmiHeader.biSizeImage = 0;

   SetStretchBltMode(hdcNew, COLORONCOLOR);
   StretchDIBits(hdcNew,
           0, 0, m_width, m_height,
           0, 0, m_width, m_height,
           m_pdsBuffer->data(), &bmi, DIB_RGB_COLORS, SRCCOPY);

   SelectObject(hdcNew, hbmOld);
   DeleteDC(hdcNew);
   ReleaseDC(NULL,hdcScreen);
}

void Texture::GetTextureDC(HDC *pdc)
{
    EnsureHBitmap();
    *pdc = CreateCompatibleDC(NULL);
    m_oldHBM = (HBITMAP)SelectObject(*pdc, m_hbmGDIVersion);
}

void Texture::ReleaseTextureDC(HDC dc)
{
    SelectObject(dc, m_oldHBM);
    DeleteDC(dc);
}



void Texture::CreateFromResource(const int id, int * const pwidth, int * const pheight)
{
   HBITMAP hbm = (HBITMAP)LoadImage(g_hinst, MAKEINTRESOURCE(id), IMAGE_BITMAP, 0, 0, LR_CREATEDIBSECTION);

   if (hbm == NULL)
   {
      m_pdsBufferColorKey=NULL;
      return;
   }

   m_pdsBufferColorKey = CreateFromHBitmap(hbm, pwidth, pheight);
}


BaseTexture* Texture::CreateFromHBitmap(HBITMAP hbm, int * const pwidth, int * const pheight)
{
   BaseTexture* pdds = MemTexture::CreateFromHBitmap(hbm);
   SetSizeFrom(pdds);
   if (pwidth) *pwidth = pdds->width();
   if (pheight) *pheight = pdds->height();
   return pdds;
}

void Texture::CreateTextureOffscreen(const int width, const int height)
{
   m_pdsBufferColorKey = new MemTexture( width, height );
   SetSizeFrom( m_pdsBufferColorKey );
}


void Texture::SetOpaque(BaseTexture* pdds)
{
    const int width = pdds->width();
    const int height = pdds->height();
    const int pitch = pdds->pitch();

    // Assume our 32 bit color structure
    BYTE *pch = pdds->data();

    for (int i=0;i<height;i++)
    {
        for (int l=0;l<width;l++)
        {
            pch[4*l + 3] = 0xff;
        }
        pch += pitch;
    }
}


void Texture::SetOpaqueBackdrop(BaseTexture* pdds, const COLORREF rgbTransparent, const COLORREF rgbBackdrop)
{
   const int width = pdds->width();
   const int height = pdds->height();
   const int lpitch = pdds->pitch();

   const unsigned int rback = (rgbBackdrop & 0x00ff0000) >> 16;
   const unsigned int gback = (rgbBackdrop & 0x0000ff00) >> 8;
   const unsigned int bback = (rgbBackdrop & 0x000000ff);

   const unsigned int rgbBd = rback | (gback << 8) | (bback << 16) | ((unsigned int)0xff << 24);

   // Assume our 32 bit color structure
   BYTE *pch = pdds->data();

   for (int i=0;i<height;i++)
   {
      for (int l=0;l<width;l++)
      {
         if ((*(unsigned int *)pch & 0xffffff) != rgbTransparent)
         {
            pch[3] = 0xff;
         }
         else
         {
            *(unsigned int *)pch = rgbBd;
         }
         pch += 4;
      }
      pch += lpitch-(width*4);
   }
}

void Texture::CreateMipMap()
{
   CreateNextMipMapLevel( m_pdsBufferColorKey );
}

BOOL Texture::SetAlpha(const COLORREF rgbTransparent)
{
    if (!m_pdsBufferColorKey)
        return FALSE;
    else
        return Texture::SetAlpha(m_pdsBufferColorKey, rgbTransparent);
}



BOOL Texture::SetAlpha(BaseTexture* pdds, const COLORREF rgbTransparent)
{
    // Set alpha of each pixel
    const int width = pdds->width();
    const int height = pdds->height();

    BOOL fTransparent = fFalse;

    const int pitch = pdds->pitch();

    const COLORREF rtrans = (rgbTransparent & 0x000000ff);
    const COLORREF gtrans = (rgbTransparent & 0x0000ff00) >> 8;
    const COLORREF btrans = (rgbTransparent & 0x00ff0000) >> 16;

    const COLORREF bgrTransparent = btrans | (gtrans << 8) | (rtrans << 16) | 0xff000000;  // color order different in DirectX texture buffer
    // Assume our 32 bit color structure

    BYTE *pch = pdds->data();
    if (rgbTransparent != NOTRANSCOLOR)
    {
        // check if image has it's own alpha channel -- compute min and max alpha
        unsigned int aMax = ((*(COLORREF *)pch) & 0xff000000)>>24;
        unsigned int aMin = ((*(COLORREF *)pch) & 0xff000000)>>24;
        for (int i=0;i<height;i++)
        {
            for (int l=0;l<width;l++)
            {
                if (((*(COLORREF *)pch) & 0xff000000)>>24 > aMax)
                    aMax = ((*(COLORREF *)pch) & 0xff000000)>>24;
                if (((*(COLORREF *)pch) & 0xff000000)>>24 < aMin)
                    aMin = ((*(COLORREF *)pch) & 0xff000000)>>24;
                pch += 4;
            }
            pch += pitch-(width*4);
        }
        slintf("amax:%d amin:%d\n",aMax,aMin);
        pch = pdds->data();

        for (int i=0;i<height;i++)
        {
            for (int l=0;l<width;l++)
            {
                const COLORREF tc = (*(COLORREF *)pch) | 0xff000000; //set to opaque
                if (tc == bgrTransparent )					// reg-blue order reversed
                {
                    *(unsigned int *)pch = 0x00000000;		// set transparent colorkey to black	and alpha transparent
                    fTransparent = fTrue;					// colorkey is true
                }
                else
                {
                    //to enable alpha uncomment these three lines (does not work with HD-Render)
                    if ((aMin == aMax) && (aMin == 255))    // if there is no alpha-channel info in the image, set to opaque
                        *(COLORREF *)pch = tc;
                    else
                        fTransparent = fTrue;   // does not work. - cupid: i need a real PC to test this.
                }
                pch += 4;
            }
            pch += pitch-(width*4);
        }
    }

    return fTransparent;
}

static const int rgfilterwindow[7][7] =
{
    1,  4,  8, 10,  8,  4,  1,
    4, 12, 25, 29, 25, 12,  4,
    8, 25, 49, 58, 49, 25,  8,
   10, 29, 58, 67, 58, 29, 10,
    8, 25, 49, 58, 49, 25,  8,
    4, 12, 25, 29, 25, 12,  4,
    1,  4,  8, 10,  8,  4,  1
};

void Texture::Blur(BaseTexture* pdds, const BYTE * const pbits, const int shadwidth, const int shadheight)
{
    if (!pbits) return;	// found this pointer to be NULL after some graphics errors

    const int width = pdds->width();
    const int height = pdds->height();
    const int pitch = pdds->pitch();

/*  int window[7][7]; // custom filter kernel
    for (int i=0;i<4;i++)
    {
      window[0][i] = i+1;
      window[0][6-i] = i+1;
      window[i][0] = i+1;
      window[6-i][0] = i+1;
    }  */

    int totalwindow = 0;
    for (int i=0;i<7;i++)
    {
        for (int l=0;l<7;l++)
        {
            //window[i][l] = window[0][l] * window[i][0];
            totalwindow += rgfilterwindow[i][l];
        }
    }

    // Gaussian Blur the sharp shadows

    const int pitchSharp = 256*3;
    BYTE *pc = pdds->data();

    for (int i=0;i<shadheight;i++)
    {
        for (int l=0;l<shadwidth;l++)
        {
            int value = 0;
            int totalvalue = totalwindow;

            for (int n=0;n<7;n++)
            {
                const int y = i+n-3;
                if(/*y>=0 &&*/ (unsigned int)y<(unsigned int)shadheight) // unsigned arithmetic trick includes check for >= zero
                {
                    const BYTE *const py = pbits + pitchSharp*y;
                    for (int m=0;m<7;m++)
                    {
                        const int x = l+m-3;
                        if (/*x>=0 &&*/ (unsigned int)x<(unsigned int)shadwidth) // dto. //!! opt.
                        {
                            value += (int)(*(py + x*3)) * rgfilterwindow[m][n];
                        }
                        else
                        {
                            totalvalue -= rgfilterwindow[m][n];
                        }
                    }
                }
                else
                {
                    for (int m=0;m<7;m++)
                    {
                        const int x = l+m-3;
                        if (/*x<0 ||*/ (unsigned int)x>=(unsigned int)shadwidth) // dto.
                        {
                            totalvalue -= rgfilterwindow[m][n];
                        }
                    }
                }
            }

            value /= totalvalue; //totalwindow;

            const unsigned int valueu = 127 + (value>>1);
            *((unsigned int*)pc) = valueu | (valueu<<8) | (valueu<<16) | (valueu<<24); // all R,G,B,A get same value
            pc += 4;
        }

        pc += pitch - shadwidth*4;
    }
}
