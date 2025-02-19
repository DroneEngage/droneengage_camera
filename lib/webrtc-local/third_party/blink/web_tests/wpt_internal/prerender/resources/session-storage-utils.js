// Copyright 2021 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function getSessionStorageKeys() {
  let keys = [];
  let txt = '';
  for (let i = 0; i < sessionStorage.length; ++i) {
    keys.push(sessionStorage.key(i));
  }
  keys.sort();
  keys.forEach((key) => {
    if (txt.length) {
      txt += ', ';
    }
    txt += key;
  });
  return txt;
}

function getNextMessage(channel) {
  return new Promise(resolve => {
    channel.addEventListener('message', e => {
      resolve(e.data);
    }, {once: true});
  });
}

// session_storage_test() is a utility function for running session storage
// related tests that open a initiator page using window.open().
function session_storage_test(testPath) {
  promise_test(async t => {
    const testChannel = new BroadcastChannel('test-channel');
    t.add_cleanup(() => {
      testChannel.close();
    });
    const gotMessage = getNextMessage(testChannel);
    const url = 'resources/' + testPath;
    window.open(url, '_blank', 'noopener');
    assert_equals(await gotMessage, 'Done');
  }, testPath);
}

// RunSessionStorageTest() is a utility function for running session storage
// related tests that requires coordinated code execution on both the initiator
// page and the prerendering page. The passed |func| function will be called
// with the following arguments:
//   - isPrerendering: Whether the |func| is called in the prerendering page.
//   - url: The URL of the prerendering page. |func| should call
//     startPrerendering(url) when |isPrerendering| is false to start the
//     prerendering.
//   - channel: A Broadcast Channel which can be used to coordinate the code
//     execution on the initiator page and the prerendering page.
//   - done: A function that should be called when the test completes
//     successfully.
async function RunSessionStorageTest(func) {
  const url = new URL(document.URL);
  url.searchParams.set('prerendering', '');
  const params = new URLSearchParams(location.search);
  // The main test page loads the initiator page, then the initiator page will
  // prerender itself with the `prerendering` parameter.
  const isPrerendering = params.has('prerendering');
  const prerenderChannel = new BroadcastChannel('prerender-channel');
  const testChannel = new BroadcastChannel('test-channel');
  window.addEventListener('unload', () => {
    prerenderChannel.close();
    testChannel.close();
  });
  try {
    await func(isPrerendering, url.toString(), prerenderChannel, () => {
      testChannel.postMessage('Done');
    })
  } catch (e) {
    testChannel.postMessage(e.toString());
  }
}
