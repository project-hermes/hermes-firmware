#include <WiFi.h>
#include <mbedtls/md.h>

#include <Utils.hpp>

String remoraID()
{
    byte shaResult[32];
    WiFi.mode(WIFI_MODE_STA);
    String unhashed_id = WiFi.macAddress();
    mbedtls_md_context_t ctx;
    mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

    const size_t payloadLength = strlen(unhashed_id.c_str());

    mbedtls_md_init(&ctx);
    mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
    mbedtls_md_starts(&ctx);
    mbedtls_md_update(&ctx, (const unsigned char *)unhashed_id.c_str(), payloadLength);
    mbedtls_md_finish(&ctx, shaResult);
    mbedtls_md_free(&ctx);

    String hash;
    for (int i = 0; i < sizeof(shaResult); i++)
    {
        char str[3];

        sprintf(str, "%02x", (int)shaResult[i]);
        hash = hash + str;
    }

    return hash;
}