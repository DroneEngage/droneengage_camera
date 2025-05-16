#ifndef CSETSESSIONDESCRIPTIONOBSERVER_H

#define CSETSESSIONDESCRIPTIONOBSERVER_H
namespace de
{
namespace stream_webrtc{
class CSetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver
{
    public:
        static CSetSessionDescriptionObserver* Create(de::CSDOCallBack *csdocallback) {
            return new rtc::RefCountedObject<CSetSessionDescriptionObserver>(csdocallback);
        }

        void OnSuccess() override;

        void OnFailure(webrtc::RTCError error) override;

         bool no = false;
    protected:
         CSetSessionDescriptionObserver (de::CSDOCallBack *csdocallback):m_csdocallback(csdocallback)
         {}

         de::CSDOCallBack *m_csdocallback;
};

}
}


#endif