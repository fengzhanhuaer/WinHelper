#include "tab_about.h"

#include "imgui.h"

void RenderTabAbout() {
    if (!ImGui::BeginTabItem("\xe5\x85\xb3\xe4\xba\x8e")) // 关于
        return;

    ImGui::TextUnformatted("Win Helper");
    ImGui::TextUnformatted("\xe5\x9f\xba\xe4\xba\x8e Dear ImGui + Win32 + DirectX 11"); // 基于 Dear ImGui + Win32 + DirectX 11
    ImGui::TextUnformatted("\xe7\x94\xa8\xe4\xba\x8e\xe5\xbf\xab\xe9\x80\x9f\xe8\x81\x9a\xe5\x90\x88\xe5\xb8\xb8\xe8\xa7\x81 Windows \xe7\xae\xa1\xe7\x90\x86\xe5\x85\xa5\xe5\x8f\xa3\xe3\x80\x82 "); // 用于快速聚合常见 Windows 管理入口。

    ImGui::EndTabItem();
}
