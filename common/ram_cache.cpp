#include "ram_cache.h"

using namespace nx;

size_t nx_curl_header_callback( char *, size_t size, size_t nmemb, void *) {
    //ignore headers //should actually report errors
    return size*nmemb;
}
size_t nx_curl_write_callback( char *ptr, size_t size, size_t nmemb, void *userdata) {
    CurlData *data = (CurlData *)userdata;
    uint32_t s = size * nmemb;
    uint32_t missing = data->expected - data->written;
    assert(s <= missing + 1);
    if(s > missing)
        s = missing;
    memcpy(data->buffer + data->written, ptr, s);
    data->written += s;
    //return some other number to abort.
    return size*nmemb;
}


void RamCache::loadNexus(Nexus *nexus) {
    if(nexus->isStreaming()) {
#ifdef USE_CURL
        CurlData data(new char[sizeof(Header)], sizeof(Header));
        int readed = getCurl(nexus->url.c_str(), data, 0, sizeof(Header));
        if(readed == -1) {
            cerr << "Failed loading: " << nexus->url << endl;
            return;
        }
        assert(data.written == sizeof(Header));
        try {
            nexus->loadHeader(data.buffer);
            delete []data.buffer;
        } catch (const char *error) {
            cerr << "Error: " << error << endl;
            return;
        }

        uint32_t index_size = nexus->indexSize();
        data.buffer = new char[index_size];
        data.written = 0;
        data.expected = index_size;

        readed = getCurl(nexus->url.c_str(), data, sizeof(Header), index_size + sizeof(Header));
        assert(data.written == data.expected);
        if(readed == -1) {
            cerr << "Failed loading: " << nexus->url << endl;
            return;
        }

        nexus->loadIndex(data.buffer);

        delete []data.buffer;
#else
        throw "Compiled without curl library";
#endif
    } else {
        try {
            nexus->loadHeader();
            nexus->loadIndex();
        } catch (const char *error) {
            cerr << "Error: " << error << endl;
            return;
        }

    }
}

int RamCache::get(nx::Token *in) {
    //mt::sleep_ms(1000);
    if(in->nexus->isStreaming()) {
#ifdef USE_CURL
        Node &node = in->nexus->nodes[in->node];
        NodeData &nodedata = in->nexus->nodedata[in->node];
        nodedata.memory = new char[node.getSize()];
        memset(nodedata.memory, 0, node.getSize());

        CurlData data(nodedata.memory, node.getSize());
        int32_t readed = getCurl(in->nexus->url.c_str(), data, node.getBeginOffset(), node.getEndOffset());
        return readed;
#else
        throw "Compiled without curl library";
#endif
    } else
        return in->nexus->loadRam(in->node);
}

int drop(nx::Token *in) {
    if(in->nexus->isStreaming()) {
        NodeData &nodedata = in->nexus->nodedata[in->node];
        delete []nodedata.memory;
        return in->nexus->nodes[in->node].getSize();
    } else
        return in->nexus->dropRam(in->node);
}


uint64_t RamCache::getCurl(const char *url, CurlData &data, uint64_t start, uint64_t end) {
#ifdef USE_CURL
    struct curl_slist *header = NULL;
    stringstream range;

    range  << "Range: bytes=" << start << "-" << end;
    header = curl_slist_append(header, range.str().c_str());

    //curl_easy_setopt(curl, CURLOPT_HEADER, true);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, header);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, nx_curl_header_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&data);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, nx_curl_write_callback);

    CURLcode res = curl_easy_perform(curl);
    if(res != CURLE_OK)
        return -1;

    return data.written;
#endif
    return -1;
}
