#include "programs/cpu_tlp_shared_cache/views/PE0MemView.h"
#include "imgui.h"

void PE0MemView::render() {
    // ÚNICO elemento: la tabla con scroll si lo requiere
    m_table.render("##PE0_MemTable");
}

// -------------- API de datos (interno, no UI) -----------------
void PE0MemView::setRow(int row, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setRow(row, tag, data, state);
}
void PE0MemView::setTag(int row, const std::string& tag) { m_table.setTag(row, tag); }
void PE0MemView::setData(int row, const std::string& data) { m_table.setData(row, data); }
void PE0MemView::setState(int row, const std::string& state) { m_table.setState(row, state); }

void PE0MemView::setBySetWay(int setIndex, int wayIndex, const std::string& tag, const std::string& data, const std::string& state) {
    m_table.setBySetWay(setIndex, wayIndex, tag, data, state);
}
void PE0MemView::setTagBySetWay(int setIndex, int wayIndex, const std::string& tag) { m_table.setTagBySetWay(setIndex, wayIndex, tag); }
void PE0MemView::setDataBySetWay(int setIndex, int wayIndex, const std::string& data) { m_table.setDataBySetWay(setIndex, wayIndex, data); }
void PE0MemView::setStateBySetWay(int setIndex, int wayIndex, const std::string& state) { m_table.setStateBySetWay(setIndex, wayIndex, state); }
