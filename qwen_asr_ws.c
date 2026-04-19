#include "qwen_asr.h"
#include "qwen_asr_ws.h"
#include <libwebsockets.h>

float *convert_pcm_to_float(const uint8_t *data, size_t size, int *out_n_samples) {
    if (size < 2) return NULL;

    int n_frames = (int)(size / 2);
    float *samples = (float *)malloc(n_frames * sizeof(float));
    if (!samples) return NULL;

    const int16_t *src = (const int16_t *)data;
    for (int i = 0; i < n_frames; i++) {
        // Convert s16le to float (-1.0 to 1.0)
        samples[i] = src[i] / 32768.0f;
    }

    *out_n_samples = n_frames;
    return samples;
}

static int ws_callback(struct lws *wsi, enum lws_callback_reasons reason,
                       void *user, void *in, size_t len) {
    
    per_session_data_t *pss = (per_session_data_t *)user;
    qwen_ctx_t *ctx = (qwen_ctx_t *)lws_context_user(lws_get_context(wsi));

    switch (reason) {
        case LWS_CALLBACK_RECEIVE:
            if (ctx && in && len > 0) {
                uint8_t *new_buf = realloc(pss->msg_buffer, pss->msg_len + len);
                if (!new_buf) return -1;
                
                pss->msg_buffer = new_buf;
                memcpy(pss->msg_buffer + pss->msg_len, in, len);
                pss->msg_len += len;

                if (lws_is_final_fragment(wsi)) {
                    int n_samples = 0;
                    float *samples = convert_pcm_to_float(pss->msg_buffer, pss->msg_len, &n_samples);                    
                    if (samples) {
                        char *text = qwen_transcribe_audio(ctx, samples, n_samples);                        
                        if (text) {
                            size_t text_len = strlen(text);
                            unsigned char *out_buf = malloc(LWS_PRE + text_len);
                            if (out_buf) {
                                memcpy(&out_buf[LWS_PRE], text, text_len);
                                lws_write(wsi, &out_buf[LWS_PRE], text_len, LWS_WRITE_TEXT);
                                free(out_buf);
                            }
                            free(text); 
                        }
                        free(samples);
                    }

                    free(pss->msg_buffer);
                    pss->msg_buffer = NULL;
                    pss->msg_len = 0;
                }
            }
            break;

        case LWS_CALLBACK_CLOSED:
            if (pss->msg_buffer) {
                free(pss->msg_buffer);
                pss->msg_buffer = NULL;
                pss->msg_len = 0;
            }
            break;

        default:
            break;
    }
    return 0;
}

static struct lws_protocols protocols[] = {
    { "qwen-protocol", ws_callback, sizeof(per_session_data_t), 65536 },
    { NULL, NULL, 0, 0 }
};

void setupWs(qwen_ctx_t *ctx) {
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = 5000;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;
    info.user = (void *)ctx;

    struct lws_context *context = lws_create_context(&info);
    if (!context) {
        fprintf(stderr, "lws init failed\n");
        return;
    }

    printf("\nWebSocket server started on port 5000...\n");
    fflush(stdout);

    while (1) {
        lws_service(context, 50);
    }
    
    lws_context_destroy(context);
}