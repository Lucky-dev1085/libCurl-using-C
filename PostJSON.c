/* Use libcurl to POST JSON data.

Usage: PostJSON <name> <value>

curl-library mailing list thread:
'how do i post json to a https ?'
http://curl.haxx.se/mail/lib-2015-01/0048.html
*/

#include <stdio.h>
#include <stdlib.h>

// http://curl.haxx.se/download.html
#include <curl/curl.h>

// http://sourceforge.net/projects/cjson/
#include "cJSON.h"


int PostJSON(const char *name, const char *value);

int main(int argc, char *argv[])
{
    if(argc != 3)
    {
        fprintf(stderr, "Usage: PostJSON <name> <value>\n");
        return EXIT_FAILURE;
    }

    if(curl_global_init(CURL_GLOBAL_ALL))
    {
        fprintf(stderr, "Fatal: The initialization of libcurl has failed.\n");
        return EXIT_FAILURE;
    }

    if(atexit(curl_global_cleanup))
    {
        fprintf(stderr, "Fatal: atexit failed to register curl_global_cleanup.\n");
        curl_global_cleanup();
        return EXIT_FAILURE;
    }

    if(!PostJSON(argv[1], argv[2]))
    {
        fprintf(stderr, "Fatal: PostJSON failed.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

// Returns TRUE on success, FALSE on failure.
int PostJSON(const char *name, const char *value)
{
    int retcode = FALSE;
    cJSON *root = NULL, *item = NULL;
    char *json = NULL;
    CURL *curl = NULL;
    CURLcode res = CURLE_FAILED_INIT;
    struct curl_slist *headers = NULL;
    char agent[1024] = { 0, };

    root = cJSON_CreateObject();
    if(!root)
    {
        fprintf(stderr, "cJSON_CreateObject failed.\n");
        goto cleanup;
    }

    cJSON_AddStringToObject(root, name, value);
    /* There is no return value for cJSON_AddStringToObject. To make sure it succeeded we try
    retrieving what we added. If you are adding a lot of values it would be much more efficient to
    write a malloc wrapper that sets a global fail flag if malloc fails; point cJSON to your wrapper
    via cJSON_InitHooks, and then test the fail flag after adding all your values.
    */
    item = cJSON_GetObjectItem(root, name);
    if(item && item->string &&
#ifdef _WIN32
        _stricmp(
#else
        strcasecmp(
#endif
            item->valuestring, value))
    {
        fprintf(stderr, "cJSON_AddStringToObject failed.\n");
        goto cleanup;
    }

    json = cJSON_PrintUnformatted(root);
    if(!json)
    {
        fprintf(stderr, "cJSON_PrintUnformatted failed.\n");
        goto cleanup;
    }

    curl = curl_easy_init();
    if(!curl)
    {
        fprintf(stderr, "curl_easy_init failed.\n");
        goto cleanup;
    }

    /* CURLOPT_CAINFO
    To verify SSL sites you may need to load a bundle of certificates.

    You can download the default bundle here:
    https://raw.githubusercontent.com/bagder/ca-bundle/master/ca-bundle.crt

    However your SSL backend might use a database in addition to or instead of this.
    http://curl.haxx.se/docs/ssl-compared.html
    */
    curl_easy_setopt(curl, CURLOPT_CAINFO, "curl-ca-bundle.crt");

#ifdef _WIN32
    _snprintf(
#else
    snprintf(
#endif
        agent, sizeof agent, "libcurl/%s", curl_version_info(CURLVERSION_NOW)->version);
    agent[sizeof agent - 1] = 0;
    curl_easy_setopt(curl, CURLOPT_USERAGENT, agent);

    headers = curl_slist_append(headers, "Expect:");
    headers = curl_slist_append(headers, "Content-Type: application/json; charset=utf-8");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json); 
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, -1L); 

    // This is a test server, it fakes a reply as if the json object were created.
    curl_easy_setopt(curl, CURLOPT_URL, "http://jsonplaceholder.typicode.com/posts");

    curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK)
    {
        fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
        goto cleanup;
    }

    retcode = TRUE;
cleanup:
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    cJSON_Delete(root);
    free(json);
    return retcode;
}
