#ifndef CSETSESSIONDESCRIPTIONOBSERVER_H

#define CSETSESSIONDESCRIPTIONOBSERVER_H
namespace uavos
{
namespace stream_webrtc{
class CSetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver
{
    public:
        static CSetSessionDescriptionObserver* Create(uavos::CSDOCallBack *csdocallback) {
            return new rtc::RefCountedObject<CSetSessionDescriptionObserver>(csdocallback);
        }

        void OnSuccess() override;

        void OnFailure(webrtc::RTCError error) override;

         bool no = false;
    protected:
         CSetSessionDescriptionObserver (uavos::CSDOCallBack *csdocallback):m_csdocallback(csdocallback)
         {}

         uavos::CSDOCallBack *m_csdocallback;
};

}
}


#endif