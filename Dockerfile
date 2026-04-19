# Build
FROM debian:bookworm-slim AS builder

RUN apt-get update && apt-get install -y \
    build-essential \
    libwebsockets-dev \
    libssl-dev \
    libopenblas-dev \
    pkg-config \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /build
COPY . .
RUN make blas

# Runtime
FROM debian:bookworm-slim

RUN apt-get update && apt-get install -y --no-install-recommends \
    libwebsockets17 \
    libopenblas0-pthread \
    libssl3 \
    ca-certificates \
    curl \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY --from=builder /build/qwen_asr .

COPY docker-entrypoint.sh download_model.sh ./

RUN chmod +x docker-entrypoint.sh download_model.sh

# Envs default
ENV MODEL_SIZE large
ENV MODEL_DIR /app/models

EXPOSE 5000

ENTRYPOINT ["/bin/bash", "/app/docker-entrypoint.sh"]