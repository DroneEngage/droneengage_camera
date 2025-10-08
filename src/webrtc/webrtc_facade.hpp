#ifndef FCB_FACADE_H_
#define FCB_FACADE_H_

#include "../common.h"

#include "../de_common/de_databus/de_facade_base.hpp"

#include "../de_common/helpers/json_nlohmann.hpp"
using Json_de = nlohmann::json;


namespace de
{
namespace fcb
{
    /**
     * @brief Mainly this class handles communication received from FCB or andruav fcb unit internal status
     * and sends it to communicator and server.
     * 
     */
    class CFCBFacade : public de::comm::CFacade_Base
    {
        public:

            static CFCBFacade& getInstance()
            {
                static CFCBFacade    instance; // Guaranteed to be destroyed.
                                                // Instantiated on first use.
                return instance;
            }
            CFCBFacade(CFCBFacade const&)           = delete;
            void operator=(CFCBFacade const&)       = delete;

        private:

            CFCBFacade() 
            {
            }

            
        public:
            
            ~CFCBFacade ()
            {

            }
        
        
        public:

        void Hangup(const std::string& senderPartyID, const std::string& PartyID, const std::string& channel);
        void OnIceCandidate(const de::STRUCT_SESSION_INFO sessionInfo, const std::string& PartyID, const webrtc::IceCandidateInterface* const candidate);
    };
}
}

#endif