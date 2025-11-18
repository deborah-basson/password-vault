FROM ubuntu:24.04

LABEL maintainer="deborah.basson@epita.fr"
LABEL description="Password Vault - Secure Password Manager"
LABEL version="1.0.0"

RUN apt-get update && apt-get install -y \
    libsqlite3-0 \
    libsodium23 \
    libssl3 \
    ca-certificates \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -u 1000 -s /bin/bash vault && \
    mkdir -p /app /data && \
    chown -R vault:vault /app /data

WORKDIR /app

COPY --chown=vault:vault password_vault .
COPY --chown=vault:vault web/ ./web/
COPY --chown=vault:vault cert.pem key.pem ./

RUN chmod 500 password_vault && \
    chmod 600 cert.pem key.pem && \
    chmod 755 web

USER vault

EXPOSE 8443

VOLUME ["/data"]

HEALTHCHECK --interval=30s --timeout=3s --start-period=5s --retries=3 \
  CMD nc -z localhost 8443 || exit 1

ENTRYPOINT ["./password_vault"]