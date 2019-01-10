#ifdef _WIN32
#include "Win32/stdafx.h"
#endif
#include <DK.h>


class SampleApp : public DKApplication
{
public:
    SampleApp()
    {
    }
    ~SampleApp()
    {
    }

    void OnInitialize(void) override
    {
        DKLogD("%s", DKGL_FUNCTION_NAME);

        DKString resPath = DefaultPath(SystemPath::AppResource);
        resPath = resPath.FilePathStringByAppendingPath("Data");
        DKLog("resPath: %ls", (const wchar_t*)resPath);
        resourcePool.AddLocatorForPath(resPath);
    }
    void OnTerminate(void) override
    {
        DKLogD("%s", DKGL_FUNCTION_NAME);

        DKLogI("Memory Pool Statistics");
        size_t numBuckets = DKMemoryPoolNumberOfBuckets();
        DKMemoryPoolBucketStatus* buckets = new DKMemoryPoolBucketStatus[numBuckets];
        DKMemoryPoolQueryAllocationStatus(buckets, numBuckets);
        size_t usedBytes = 0;
        for (int i = 0; i < numBuckets; ++i)
        {
            if (buckets[i].totalChunks > 0)
            {
                DKLogI("--> %5lu:  %5lu/%5lu, usage: %.1f%%, used: %.1fKB, total: %.1fKB",
                    buckets[i].chunkSize,
                    buckets[i].usedChunks, buckets[i].totalChunks,
                    double(buckets[i].usedChunks) / double(buckets[i].totalChunks) * 100.0,
                    double(buckets[i].chunkSize * buckets[i].usedChunks) / 1024.0,
                    double(buckets[i].chunkSize * buckets[i].totalChunks) / 1024.0
                );
                usedBytes += buckets[i].chunkSize * buckets[i].usedChunks;
            }
        }
        DKLogI("MemoryPool Usage: %.1fMB / %.1fMB", double(usedBytes) / (1024 * 1024), double(DKMemoryPoolSize()) / (1024 * 1024));
        delete[] buckets;
    }

    DKResourcePool resourcePool;
};
