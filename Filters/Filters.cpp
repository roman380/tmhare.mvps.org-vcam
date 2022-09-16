#pragma warning(disable:4244)
#pragma warning(disable:4711)

#include "Message.h"

#include <streams.h>
#include <stdio.h>
#include <olectl.h>
#include <dvdmedia.h>
#include "filters.h"
#include <uuids.h>
#include <fstream>

const GUID MEDIASUBTYPE_I420 = {0x30323449,
                0x0000,
                0x0010,
                {0x80, 0x00, 0x00, 0xaa, 0x00, 0x38, 0x9b,
                 0x71} };


void sendCameraStatus(CameraStatus st)
{

    MessageCameraStatus status;
    status.payload = st;

    HANDLE hPipe;
    DWORD dwWritten;

    hPipe = CreateFile(TEXT("\\\\.\\pipe\\StbVirtualCameraPipeline"),
        GENERIC_READ | GENERIC_WRITE,
        0,
        NULL,
        OPEN_EXISTING,
        0,
        NULL);

    if (hPipe != INVALID_HANDLE_VALUE)
    {
        WriteFile(hPipe,
            (uint8_t*)&status,
            sizeof(MessageCameraStatus),
            &dwWritten,
            NULL);

        CloseHandle(hPipe);
    }
}

//////////////////////////////////////////////////////////////////////////
//  CVCam is the source filter which masquerades as a capture device
//////////////////////////////////////////////////////////////////////////
CUnknown * WINAPI CVCam::CreateInstance(LPUNKNOWN lpunk, HRESULT *phr)
{
    ASSERT(phr);
    CUnknown *punk = new CVCam(lpunk, phr);
    return punk;
}

CVCam::CVCam(LPUNKNOWN lpunk, HRESULT *phr) : 
    CSource(NAME(name.c_str()), lpunk, CLSID_VirtualCam)
{
    ASSERT(phr);
    CAutoLock cAutoLock(&m_cStateLock);
    // Create the one and only output pin
    m_paStreams = (CSourceStream **) new CVCamStream*[1];
    m_paStreams[0] = new CVCamStream(phr, this, wname.c_str());
}

HRESULT CVCam::QueryInterface(REFIID riid, void **ppv)
{
    //Forward request for IAMStreamConfig & IKsPropertySet to the pin
    if(riid == _uuidof(IAMStreamConfig) || riid == _uuidof(IKsPropertySet))
        return m_paStreams[0]->QueryInterface(riid, ppv);
    else
        return CSource::QueryInterface(riid, ppv);
}

//////////////////////////////////////////////////////////////////////////
// CVCamStream is the one and only output pin of CVCam which handles 
// all the stuff.
//////////////////////////////////////////////////////////////////////////
CVCamStream::CVCamStream(HRESULT *phr, CVCam *pParent, LPCWSTR pPinName) :
    CSourceStream(NAME(name.c_str()),phr, pParent, pPinName), m_pParent(pParent)
{
    // Set the default media type as 320x240x24@15
    GetMediaType(4, &m_mt);
    placeholder.initialize_placeholder();
}

CVCamStream::~CVCamStream()
{
} 

HRESULT CVCamStream::QueryInterface(REFIID riid, void **ppv)
{   
    // Standard OLE stuff
    if(riid == _uuidof(IAMStreamConfig))
        *ppv = (IAMStreamConfig*)this;
    else if(riid == _uuidof(IKsPropertySet))
        *ppv = (IKsPropertySet*)this;
    else
        return CSourceStream::QueryInterface(riid, ppv);

    AddRef();
    return S_OK;
}


//////////////////////////////////////////////////////////////////////////
//  This is the routine where we create the data being output by the Virtual
//  Camera device.
//////////////////////////////////////////////////////////////////////////

HRESULT CVCamStream::FillBuffer(IMediaSample *pms)
{
    nv12_scale_t scale;
    scale.dst_cx = 1920;
    scale.dst_cy = 1080;
    scale.format = TARGET_FORMAT_NV12;
    scale.src_cx = 1920;
    scale.src_cy = 1080;
    uint64_t tmp;
    if (!vq)
    {
        vq = video_queue_open();
    }

    uint32_t new_cx = cx;
    uint32_t new_cy = cy;
    uint64_t new_interval = interval;

    /* cx, cy and interval are the resolution and frame rate of the
       virtual camera _source_, ie OBS' output. Do not confuse cx / cy
       with GetCX() / GetCY() / GetInterval() which return the virtualcam
       filter output! */

    if (!vq) {
        vq = video_queue_open();
    }

    enum queue_state state = video_queue_state(vq);
    if (state != prev_state) {
        if (state == SHARED_QUEUE_STATE_READY) {
            /* The virtualcam output from OBS has started, get
               the actual cx / cy of the data stream */
            video_queue_get_info(vq, &new_cx, &new_cy,
                &new_interval);
            cx = new_cx;
            cy = new_cy;
            interval = new_interval;
        }
        else if (state == SHARED_QUEUE_STATE_STOPPING) {
            video_queue_close(vq);
            vq = nullptr;
        }

        prev_state = state;
    }

    auto rtNow = m_rtLastTime;
    m_rtLastTime += interval;
    pms->SetTime(&rtNow, &m_rtLastTime);
    pms->SetSyncPoint(TRUE);

    BYTE *pData;
    long lDataLen;
    pms->GetPointer(&pData);
    lDataLen = pms->GetSize();

    if (state == SHARED_QUEUE_STATE_READY)
    {
        if (!video_queue_read(vq, &scale, pData, &tmp))
        {
            if (!prev_frame.empty())
            {
                memcpy(pData, prev_frame.data(), prev_frame.size());
            }
            video_queue_close(vq);
        }
        else
        {
            if (prev_frame.empty())
            {
                prev_frame.reserve(lDataLen);
            }
            memcpy(prev_frame.data(), pData, prev_frame.size());
        }
    }
    else
    {
        if (placeholder.get_placeholder_ptr())
        {
            memcpy(pData, placeholder.get_placeholder_ptr(), lDataLen);
        }
        else
        {
            for (int i = 0; i < lDataLen; ++i)
                pData[i] = rand();
        }
        video_queue_close(vq);
    }

    return NOERROR;
} // FillBuffer


//
// Notify
// Ignore quality management messages sent from the downstream filter
STDMETHODIMP CVCamStream::Notify(IBaseFilter * pSender, Quality q)
{
    return E_NOTIMPL;
} // Notify

//////////////////////////////////////////////////////////////////////////
// This is called when the output format has been negotiated
//////////////////////////////////////////////////////////////////////////
HRESULT CVCamStream::SetMediaType(const CMediaType *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->Format());
    HRESULT hr = CSourceStream::SetMediaType(pmt);
    return hr;
}

// See Directshow help topic for IAMStreamConfig for details on this method
HRESULT CVCamStream::GetMediaType(int iPosition, CMediaType *pmt)
{
    if(iPosition < 0) return E_INVALIDARG;
    if(iPosition > 8) return VFW_S_NO_MORE_ITEMS;

    if(iPosition == 0) 
    {
        *pmt = m_mt;
        return S_OK;
    }

    DECLARE_PTR(VIDEOINFOHEADER, pvi, pmt->AllocFormatBuffer(sizeof(VIDEOINFOHEADER)));
    ZeroMemory(pvi, sizeof(VIDEOINFOHEADER));

    pvi->bmiHeader.biCompression = MAKEFOURCC('I', '4', '2', '0');//MAKEFOURCC('R', 'G', 'B', '4');//MAKEFOURCC('N', 'V', '1', '2');//BI_RGB;
    pvi->bmiHeader.biBitCount = 12;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = 1920;
    pvi->bmiHeader.biHeight = 1080;
    pvi->bmiHeader.biPlanes = 3;
    pvi->bmiHeader.biSizeImage  = GetBitmapSize(&pvi->bmiHeader);
    pvi->bmiHeader.biClrImportant = 0;

    pvi->AvgTimePerFrame = 1000000;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    pmt->SetType(&MEDIATYPE_Video);
    pmt->SetFormatType(&FORMAT_VideoInfo);
    pmt->SetTemporalCompression(FALSE);

    // Work out the GUID for the subtype from the header info.
    const GUID SubTypeGUID = GetBitmapSubtype(&pvi->bmiHeader);
    pmt->SetSubtype(&SubTypeGUID);
    pmt->SetSampleSize(pvi->bmiHeader.biSizeImage);
    
    return NOERROR;

} // GetMediaType

// This method is called to see if a given output format is supported
HRESULT CVCamStream::CheckMediaType(const CMediaType *pMediaType)
{
    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *)(pMediaType->Format());
    if(*pMediaType != m_mt) 
        return E_INVALIDARG;
    return S_OK;
} // CheckMediaType

// This method is called after the pins are connected to allocate buffers to stream data
HRESULT CVCamStream::DecideBufferSize(IMemAllocator *pAlloc, ALLOCATOR_PROPERTIES *pProperties)
{
    CAutoLock cAutoLock(m_pFilter->pStateLock());
    HRESULT hr = NOERROR;

    VIDEOINFOHEADER *pvi = (VIDEOINFOHEADER *) m_mt.Format();
    pProperties->cBuffers = 1;
    pProperties->cbBuffer = pvi->bmiHeader.biSizeImage;

    ALLOCATOR_PROPERTIES Actual;
    hr = pAlloc->SetProperties(pProperties,&Actual);

    if(FAILED(hr)) return hr;
    if(Actual.cbBuffer < pProperties->cbBuffer) return E_FAIL;

    return NOERROR;
} // DecideBufferSize

// Called when graph is run
HRESULT CVCamStream::OnThreadCreate()
{
    m_rtLastTime = 0;
    sendCameraStatus(CameraStatus::RUN);
    return NOERROR;
} // OnThreadCreate

HRESULT CVCamStream::OnThreadDestroy(void)
{
    sendCameraStatus(CameraStatus::STOP);
    return NOERROR;
}


//////////////////////////////////////////////////////////////////////////
//  IAMStreamConfig
//////////////////////////////////////////////////////////////////////////

HRESULT STDMETHODCALLTYPE CVCamStream::SetFormat(AM_MEDIA_TYPE *pmt)
{
    DECLARE_PTR(VIDEOINFOHEADER, pvi, m_mt.pbFormat);
    m_mt = *pmt;
    IPin* pin; 
    ConnectedTo(&pin);
    if(pin)
    {
        IFilterGraph *pGraph = m_pParent->GetGraph();
        pGraph->Reconnect(this);
    }
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetFormat(AM_MEDIA_TYPE **ppmt)
{
    *ppmt = CreateMediaType(&m_mt);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetNumberOfCapabilities(int *piCount, int *piSize)
{
    *piCount = 8;
    *piSize = sizeof(VIDEO_STREAM_CONFIG_CAPS);
    return S_OK;
}

HRESULT STDMETHODCALLTYPE CVCamStream::GetStreamCaps(int iIndex, AM_MEDIA_TYPE **pmt, BYTE *pSCC)
{
    *pmt = CreateMediaType(&m_mt);
    DECLARE_PTR(VIDEOINFOHEADER, pvi, (*pmt)->pbFormat);

    pvi->bmiHeader.biCompression = MAKEFOURCC('I', '4', '2', '0');// MAKEFOURCC('R', 'G', 'B', '4');//MAKEFOURCC('N', 'V', '1', '2');//BI_RGB;
    pvi->bmiHeader.biBitCount = 12;
    pvi->bmiHeader.biSize       = sizeof(BITMAPINFOHEADER);
    pvi->bmiHeader.biWidth = 1920;
    pvi->bmiHeader.biHeight = 1080;
    pvi->bmiHeader.biPlanes = 3;
    pvi->bmiHeader.biSizeImage = pvi->bmiHeader.biWidth * pvi->bmiHeader.biHeight * 3 / 2;//GetBitmapSize(&pvi->bmiHeader);
    //pvi->bmiHeader.biClrImportant = 0;

    SetRectEmpty(&(pvi->rcSource)); // we want the whole image area rendered.
    SetRectEmpty(&(pvi->rcTarget)); // no particular destination rectangle

    //pvi->rcSource.right = pvi->bmiHeader.biWidth;
    //pvi->rcSource.bottom = pvi->bmiHeader.biHeight;
    //pvi->rcTarget = pvi->rcSource;

    (*pmt)->majortype = MEDIATYPE_Video;
    (*pmt)->subtype = MEDIASUBTYPE_I420;//MEDIASUBTYPE_RGB24;//MEDIASUBTYPE_RGB32;//MEDIASUBTYPE_NV12; //MEDIASUBTYPE_H264;
    (*pmt)->formattype = FORMAT_VideoInfo;
    //(*pmt)->bTemporalCompression = FALSE;
    (*pmt)->bFixedSizeSamples = true;
    (*pmt)->lSampleSize = pvi->bmiHeader.biSizeImage;
    (*pmt)->cbFormat = sizeof(VIDEOINFOHEADER);
    
    DECLARE_PTR(VIDEO_STREAM_CONFIG_CAPS, pvscc, pSCC);
    
    pvscc->guid = FORMAT_VideoInfo;
    pvscc->VideoStandard = AnalogVideo_None;
    pvscc->InputSize.cx = pvi->bmiHeader.biWidth;
    pvscc->InputSize.cy = pvi->bmiHeader.biHeight;
    pvscc->MinCroppingSize = pvscc->InputSize;
    pvscc->MaxCroppingSize = pvscc->InputSize;
    pvscc->CropGranularityX = 1;//pvscc->InputSize.cx;
    pvscc->CropGranularityY = 1;//pvscc->InputSize.cy;
    pvscc->CropAlignX = 0;
    pvscc->CropAlignY = 0;

    pvscc->MinOutputSize = pvscc->InputSize;
    pvscc->MaxOutputSize = pvscc->InputSize;
    pvscc->OutputGranularityX = 1;
    pvscc->OutputGranularityY = 1;
    pvscc->StretchTapsX = 0;
    pvscc->StretchTapsY = 0;
    pvscc->ShrinkTapsX = 0;
    pvscc->ShrinkTapsY = 0;
    pvscc->MinFrameInterval = 200000;   //50 fps
    pvscc->MaxFrameInterval = 50000000; // 0.2 fps
    pvscc->MinBitsPerSecond = (1920 * 1080 * 3 * 8) / 5;
    pvscc->MaxBitsPerSecond = (1920 * 1080 * 3 * 8) * 5;

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////
// IKsPropertySet
//////////////////////////////////////////////////////////////////////////


HRESULT CVCamStream::Set(REFGUID guidPropSet, DWORD dwID, void *pInstanceData, 
                        DWORD cbInstanceData, void *pPropData, DWORD cbPropData)
{// Set: Cannot set any properties.
    return E_NOTIMPL;
}

// Get: Return the pin category (our only property). 
HRESULT CVCamStream::Get(
    REFGUID guidPropSet,   // Which property set.
    DWORD dwPropID,        // Which property in that set.
    void *pInstanceData,   // Instance data (ignore).
    DWORD cbInstanceData,  // Size of the instance data (ignore).
    void *pPropData,       // Buffer to receive the property data.
    DWORD cbPropData,      // Size of the buffer.
    DWORD *pcbReturned     // Return the size of the property.
)
{
    if (guidPropSet != AMPROPSETID_Pin)             return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY)        return E_PROP_ID_UNSUPPORTED;
    if (pPropData == NULL && pcbReturned == NULL)   return E_POINTER;
    
    if (pcbReturned) *pcbReturned = sizeof(GUID);
    if (pPropData == NULL)          return S_OK; // Caller just wants to know the size. 
    if (cbPropData < sizeof(GUID))  return E_UNEXPECTED;// The buffer is too small.
        
    *(GUID *)pPropData = PIN_CATEGORY_CAPTURE;
    return S_OK;
}

// QuerySupported: Query whether the pin supports the specified property.
HRESULT CVCamStream::QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD *pTypeSupport)
{
    if (guidPropSet != AMPROPSETID_Pin) return E_PROP_SET_UNSUPPORTED;
    if (dwPropID != AMPROPERTY_PIN_CATEGORY) return E_PROP_ID_UNSUPPORTED;
    // We support getting this property, but not setting it.
    if (pTypeSupport) *pTypeSupport = KSPROPERTY_SUPPORT_GET; 
    return S_OK;
}
