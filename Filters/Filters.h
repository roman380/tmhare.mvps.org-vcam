#pragma once

#define DECLARE_PTR(type, ptr, expr) type* ptr = (type*)(expr);

EXTERN_C const GUID CLSID_VirtualCam;

#include "shared-memory-queue.h"
#include "Placeholder.h"
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

static const std::string name = "ShareTheBoard Virtual Camera";
static const auto wname = std::wstring(name.begin(), name.end());

class CVCamStream;
class CVCam : public CSource
{
public:
    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    static CUnknown* WINAPI CreateInstance(LPUNKNOWN lpunk, HRESULT* phr);
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);

    IFilterGraph* GetGraph() { return m_pGraph; }

private:
    CVCam(LPUNKNOWN lpunk, HRESULT* phr);
};

class CVCamStream : public CSourceStream, public IAMStreamConfig, public IKsPropertySet
{
public:

    //////////////////////////////////////////////////////////////////////////
    //  IUnknown
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP QueryInterface(REFIID riid, void** ppv);
    STDMETHODIMP_(ULONG) AddRef() { return GetOwner()->AddRef(); }                                                          \
        STDMETHODIMP_(ULONG) Release() { return GetOwner()->Release(); }

    //////////////////////////////////////////////////////////////////////////
    //  IQualityControl
    //////////////////////////////////////////////////////////////////////////
    STDMETHODIMP Notify(IBaseFilter* pSender, Quality q);

    //////////////////////////////////////////////////////////////////////////
    //  IAMStreamConfig
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE SetFormat(AM_MEDIA_TYPE* pmt);
    HRESULT STDMETHODCALLTYPE GetFormat(AM_MEDIA_TYPE** ppmt);
    HRESULT STDMETHODCALLTYPE GetNumberOfCapabilities(int* piCount, int* piSize);
    HRESULT STDMETHODCALLTYPE GetStreamCaps(int iIndex, AM_MEDIA_TYPE** pmt, BYTE* pSCC);

    //////////////////////////////////////////////////////////////////////////
    //  IKsPropertySet
    //////////////////////////////////////////////////////////////////////////
    HRESULT STDMETHODCALLTYPE Set(REFGUID guidPropSet, DWORD dwID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData);
    HRESULT STDMETHODCALLTYPE Get(REFGUID guidPropSet, DWORD dwPropID, void* pInstanceData, DWORD cbInstanceData, void* pPropData, DWORD cbPropData, DWORD* pcbReturned);
    HRESULT STDMETHODCALLTYPE QuerySupported(REFGUID guidPropSet, DWORD dwPropID, DWORD* pTypeSupport);

    //////////////////////////////////////////////////////////////////////////
    //  CSourceStream
    //////////////////////////////////////////////////////////////////////////
    CVCamStream(HRESULT* phr, CVCam* pParent, LPCWSTR pPinName);
    ~CVCamStream();

    HRESULT FillBuffer(IMediaSample* pms);
    HRESULT DecideBufferSize(IMemAllocator* pIMemAlloc, ALLOCATOR_PROPERTIES* pProperties);
    HRESULT CheckMediaType(const CMediaType* pMediaType);
    HRESULT GetMediaType(int iPosition, CMediaType* pmt);
    HRESULT SetMediaType(const CMediaType* pmt);
    HRESULT OnThreadCreate(void);
    HRESULT OnThreadDestroy(void);

private:
    CVCam* m_pParent;
    REFERENCE_TIME m_rtLastTime;
    HBITMAP m_hLogoBmp;
    CCritSec m_cSharedState;
    IReferenceClock* m_pClock;
    video_queue_t* vq = nullptr;

    enum queue_state prev_state = SHARED_QUEUE_STATE_INVALID;
    uint32_t cx = 1920;
    uint32_t cy = 1080;
    uint64_t interval = 333333ULL;
    std::vector<uint8_t> prev_frame;
    Placeholder placeholder;
    uint32_t id;
    std::unique_ptr<std::thread> keep_alive_thread;
    bool is_keep_alive{ true };
    std::mutex m;
    std::condition_variable cv;
};


