#include "programs/cpu_tlp_shared_cache/views/PE1MemView.h"
#include "imgui.h"

void PE1MemView::render() {
    m_table.render("##PE1_MemTable");
}

// ---- Setters
void PE1MemView::setRow(int row, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setRow(row, tag, data, state);
}
void PE1MemView::setTag(int row, const std::string& tag) { m_table.setTag(row, tag); }
void PE1MemView::setData(int row, const std::string& data) { m_table.setData(row, data); }
void PE1MemView::setState(int row, const std::string& state) { m_table.setState(row, state); }

void PE1MemView::setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setBySetWay(setIndex, wayIndex, tag, data, state);
}
void PE1MemView::setTagBySetWay(int setIndex, int wayIndex, const std::string& tag) { m_table.setTagBySetWay(setIndex, wayIndex, tag); }
void PE1MemView::setDataBySetWay(int setIndex, int wayIndex, const std::string& data) { m_table.setDataBySetWay(setIndex, wayIndex, data); }
void PE1MemView::setStateBySetWay(int setIndex, int wayIndex, const std::string& state) { m_table.setStateBySetWay(setIndex, wayIndex, state); }
