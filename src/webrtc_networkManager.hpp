/*
 *  Copyright 2009 The WebRTC Project Authors. All rights reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */

#ifndef CNETWORK_MANAGER_H_
#define CNETWORK_MANAGER_H_



namespace de {

const int kFakeIPv4NetworkPrefixLength = 24;
const int kFakeIPv6NetworkPrefixLength = 64;

// Fake network manager that allows us to manually specify the IPs to use.
class CNetworkManager : public rtc::FakeNetworkManager {
 public:
  CNetworkManager() {}

  // void StartUpdating () override
  // {
  //   super->StartUpdating();
  //   std::cout << __FILE__ << __FUNCTION__ << " StartUpdating" << std::endl;
  // }
};

}  // namespace rtc

#endif  // RTC_BASE_FAKENETWORK_H_
