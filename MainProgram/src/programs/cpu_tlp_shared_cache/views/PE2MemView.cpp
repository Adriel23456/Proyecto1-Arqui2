#include "programs/cpu_tlp_shared_cache/views/PE2MemView.h"
#include "imgui.h"

void PE2MemView::render() {
    m_table.render("##PE2_MemTable");
}

void PE2MemView::setRow(int row, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setRow(row, tag, data, state);
}
void PE2MemView::setTag(int row, const std::string& tag) { m_table.setTag(row, tag); }
void PE2MemView::setData(int row, const std::string& data) { m_table.setData(row, data); }
void PE2MemView::setState(int row, const std::string& state) { m_table.setState(row, state); }

void PE2MemView::setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setBySetWay(setIndex, wayIndex, tag, data, state);
}
void PE2MemView::setTagBySetWay(int setIndex, int wayIndex, const std::string& tag) { m_table.setTagBySetWay(setIndex, wayIndex, tag); }
void PE2MemView::setDataBySetWay(int setIndex, int wayIndex, const std::string& data) { m_table.setDataBySetWay(setIndex, wayIndex, data); }
void PE2MemView::setStateBySetWay(int setIndex, int wayIndex, const std::string& state) { m_table.setStateBySetWay(setIndex, wayIndex, state); }
