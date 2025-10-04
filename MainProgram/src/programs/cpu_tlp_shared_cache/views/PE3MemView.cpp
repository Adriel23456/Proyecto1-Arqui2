#include "programs/cpu_tlp_shared_cache/views/PE3MemView.h"
#include "imgui.h"

void PE3MemView::render() {
    m_table.render("##PE3_MemTable");
}

void PE3MemView::setRow(int row, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setRow(row, tag, data, state);
}
void PE3MemView::setTag(int row, const std::string& tag) { m_table.setTag(row, tag); }
void PE3MemView::setData(int row, const std::string& data) { m_table.setData(row, data); }
void PE3MemView::setState(int row, const std::string& state) { m_table.setState(row, state); }

void PE3MemView::setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setBySetWay(setIndex, wayIndex, tag, data, state);
}
void PE3MemView::setTagBySetWay(int setIndex, int wayIndex, const std::string& tag) { m_table.setTagBySetWay(setIndex, wayIndex, tag); }
void PE3MemView::setDataBySetWay(int setIndex, int wayIndex, const std::string& data) { m_table.setDataBySetWay(setIndex, wayIndex, data); }
void PE3MemView::setStateBySetWay(int setIndex, int wayIndex, const std::string& state) { m_table.setStateBySetWay(setIndex, wayIndex, state); }
