#pragma once
#include "programs/cpu_tlp_shared_cache/views/ICpuTLPView.h"
#include "programs/cpu_tlp_shared_cache/widgets/MemCacheTable.h"

class PE2MemView : public ICpuTLPView {
public:
    void render() override;

    // API específica de PE2
    void setRow(int row, const std::string& tag, const std::string& data, const std::string& state);
    void setTag(int row, const std::string& tag);
    void setData(int row, const std::string& data);
    void setState(int row, const std::string& state);

    void setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state);
    void setTagBySetWay(int setIndex, int wayIndex, const std::string& tag);
    void setDataBySetWay(int setIndex, int wayIndex, const std::string& data);
    void setStateBySetWay(int setIndex, int wayIndex, const std::string& state);

private:
    MemCacheTable m_table;
};
