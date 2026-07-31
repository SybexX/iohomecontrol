#include <deque>
#include <vector>
#include <Arduino.h>
#include <log_buffer.h>
#include <user_config.h>

#if defined(WEBSERVER)
#include <web_server_handler.h>
#endif
#if defined(SYSLOG)
#include <syslog_helper.h>
#endif

namespace {
    std::deque<String> logDeque;
    const size_t MAX_LOG_ENTRIES = 50;

#if defined(WEBSERVER)
    // Keeps the WS log stream from flooding: always let through the events
    // that matter (RF/boot activity), always drop debug/verbose chatter, and
    // rate-limit everything else to at most one broadcast per 50ms.
    bool shouldBroadcastLog(const String &msg) {
        if (msg.length() == 0) {
            return false;
        }

        if (msg.indexOf("2W") >= 0 ||
            msg.indexOf("1W") >= 0 ||
            msg.indexOf("Radio RX") >= 0 ||
            msg.indexOf("Boot reset reason") >= 0 ||
            msg.indexOf("Last crash marker") >= 0) {
            return true;
        }

        if (msg.startsWith("[D]") || msg.startsWith("[V]")) {
            return false;
        }

        if (msg.indexOf("TX: TX-RX DONE") >= 0 ||
            msg.indexOf("State:") >= 0) {
            return false;
        }

        static unsigned long lastBroadcastMs = 0;
        const unsigned long now = millis();
        if (now - lastBroadcastMs < 50) {
            return false;
        }
        lastBroadcastMs = now;
        return true;
    }
#endif
}

void addLogMessage(const String &msg) {
    if (logDeque.size() >= MAX_LOG_ENTRIES) {
        logDeque.pop_front();
    }
    logDeque.push_back(msg);
#if defined(WEBSERVER)
    if (shouldBroadcastLog(msg)) {
        broadcastLog(msg);
    }
#endif
#if defined(SYSLOG)
    sendSyslog(msg);
#endif
}

std::vector<String> getLogMessages() {
    return std::vector<String>(logDeque.begin(), logDeque.end());
}
