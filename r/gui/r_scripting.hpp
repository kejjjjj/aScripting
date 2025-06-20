#pragma once

#include "r/gui/r_gui.hpp"

class CScriptingWindow : public CGuiElement
{
public:
	CScriptingWindow(const std::string& id);
	~CScriptingWindow() = default;

	void* GetRender() override {
		union {
			void (CScriptingWindow::* memberFunction)();
			void* functionPointer;
		} converter{};
		converter.memberFunction = &CScriptingWindow::Render;
		return converter.functionPointer;
	}

	void Render() override;

private:


	size_t m_uSelectedPlayback = {};
	float m_fBrushVolume = 500.f;
};
