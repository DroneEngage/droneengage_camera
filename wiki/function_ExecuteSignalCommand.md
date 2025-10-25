`ExecuteSignalCommand` is a method in the `CWEBRTC_Plugin` class that processes incoming WebRTC signaling messages.

It acts as a central dispatcher for WebRTC session control commands (like offer, answer, candidate, hangup) received over a signaling channel, routing them to appropriate internal handlers based on message content.

---

### Definition

```cpp
500:553:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_camera_2025/src/webrtc_plugin.cpp
void de::CWEBRTC_Plugin::ExecuteSignalCommand(const Json_de &jMsg)
{
    // Extract command fields
    const Json_de cmd = jMsg[ANDRUAV_PROTOCOL_MESSAGE_CMD];
    const Json_de w = cmd["w"];
    std::string number = w["number"].get<std::string>();
    std::string channel = w["channel"].get<std::string>();
    std::string sender = jMsg[ANDRUAV_PROTOCOL_SENDER].get<std::string>();
    std::string sessionID = number + channel;

    // Look up session info
    const de::STRUCT_SESSION_INFO * sessionInfo = findSessionInfoBySessionID(sessionID.c_str());
    if (sessionInfo == NULL) {
        std::cout << "Key " << sessionID << " not found" << std::endl;
    } else {
        std::cout << "Key " << sessionID << " found" << std::endl;
    }

    Json_de const packet = w["packet"];

    // Dispatch based on packet content
    if ((packet.contains("joinme")) && (packet["joinme"].get<bool>() == true)) {
        SendOffer(sender, sessionID, number, channel);
        return;
    }

    if ((packet.contains("type")) && (packet["type"].get<std::string>() == "answer")) {
        ProcessAnswer(sender, sessionID, number, channel, packet);
        return;
    }

    if (packet.contains("candidate")) {
        ProcessCandidate(sender, sessionID, number, channel, packet);
        return;
    }

    if ((packet.contains("hangup")) && (packet["hangup"].get<bool>() == true)) {
        Hangup(sender, sessionID, number, channel);
        return;
    }
}
```

- **Params**:  
  - `jMsg` (`const Json_de &`): A JSON object containing a signaling message, expected to include sender ID, command structure, and packet data.
- **Side effects**:  
  - May initiate WebRTC SDP offer via `SendOffer`.  
  - May process SDP answer via `ProcessAnswer`.  
  - May add ICE candidate via `ProcessCandidate`.  
  - May terminate a session via `Hangup`.  
  - Logs session lookup status (found/not found).
- **Returns**: `void` — performs actions directly without returning a value.

The method extracts a session identifier by concatenating `number` and `channel`, then checks if a corresponding session exists. Based on the content of the `packet` field, it routes the message to one of several handling functions that manage different stages of WebRTC peer connection setup and teardown.

---

### Example Usages

The primary usage of `ExecuteSignalCommand` occurs in `main.cpp`, where it is called to handle incoming signaling messages from a network or messaging layer.

```cpp
487:498:/home/mhefny/TDisk/public_versions/drone_engage/drone_engage_camera_2025/src/main.cpp
// Handle signaling message
std::cout << "INFO: TYPE_AndruavMessage_Signaling" << std::endl;

if (!jMsg.contains(ANDRUAV_PROTOCOL_SENDER)) {
    return; // Must have sender
}

cWEBRTC_Plugin.ExecuteSignalCommand(jMsg);  // Dispatch signaling command
```

This shows that `ExecuteSignalCommand` is the entry point for all signaling logic in the WebRTC plugin. It is invoked whenever a message of type `TYPE_AndruavMessage_Signaling` is received, ensuring that session control commands are properly interpreted and acted upon.

Overall, this function is **centrally used** in the system's WebRTC signaling flow, acting as a router. Despite being defined in `webrtc_plugin.cpp`, it is only directly called from `main.cpp`, making it a key interface between the application’s core message loop and the WebRTC session management subsystem.

---

### Notes

- The `sessionID` is constructed by simple string concatenation (`number + channel`), which assumes no delimiter collision — this could be fragile if either field contains overlapping characters.
- The function does **not handle unknown or malformed packets** — it silently ignores any `packet` that doesn’t match the expected keys (`joinme`, `type`, `candidate`, `hangup`), which may make debugging difficult.
- Session lookup uses a linear search through `m_SessionMap` via `findSessionInfoBySessionID`, which is less efficient than direct map lookup — though acceptable for small session counts.

---

### See Also

- `SendOffer`: Initiates a WebRTC connection by sending an SDP offer; called when `joinme` is true in the packet.
- `ProcessAnswer`: Handles the remote peer’s SDP answer; invoked when `packet["type"] == "answer"`.
- `ProcessCandidate`: Adds an ICE candidate to the peer connection; triggered when `candidate` field is present.
- `Hangup`: Terminates a WebRTC session and cleans up resources; called when `hangup` flag is set.
- `findSessionInfoBySessionID`: Utility function that searches `m_SessionMap` for a session by ID; used to validate session state before processing.
