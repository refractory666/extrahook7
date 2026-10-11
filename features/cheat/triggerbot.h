#pragma once

namespace mem { class Memory; }

namespace triggerbot
{
	extern bool g_trigger_enabled;
	extern bool g_trigger_active;  // true когда триггер включён и клавиша зажата/включена (для отрисовки надписи у прицела)
	extern int g_trigger_key_vk;   // VK-код клавиши бинда (по умолчанию ЛКМ)
	extern int g_trigger_mode;     // 0 = Hold, 1 = Toggle
	// Флаг: в этом тике триггер сделал выстрел
	extern bool g_trigger_fired;

	void run(mem::Memory* mem, float screen_w, float screen_h);
}
