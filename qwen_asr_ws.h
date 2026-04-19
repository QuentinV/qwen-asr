/*
 * qwen_asr_ws.h - Websocket implementation
 */

typedef struct {
    uint8_t *msg_buffer;
    size_t msg_len;
} per_session_data_t;


void setupWs(qwen_ctx_t *ctx);