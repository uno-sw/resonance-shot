#pragma once

class SdlRuntime {
  public:
    SdlRuntime();
    ~SdlRuntime();

    SdlRuntime(const SdlRuntime &) = delete;
    SdlRuntime &operator=(const SdlRuntime &) = delete;
};
