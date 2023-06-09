#pragma once
#include "pch.h"
#include "Events.h"
#include <memory>
#include <string>

class Window;

class Game : public std::enable_shared_from_this<Game>
{
	std::wstring _name;
	int _width;
	int _height;
	bool _vSync;

protected:
	std::shared_ptr<Window> _window;

	friend class Window;
	virtual void OnUpdate(UpdateEventArgs& args) {};
	virtual void OnRender(RenderEventArgs& args) {};
	virtual void OnKeyPressed(KeyEventArgs& args) {};
	virtual void OnKeyReleased(KeyEventArgs& args) {};
	virtual void OnMouseMoved(MouseMotionEventArgs& args) {};
	virtual void OnMouseButtonPressed(MouseButtonEventArgs& args) {};
	virtual void OnMouseButtonReleased(MouseButtonEventArgs& args) {};
	virtual void OnMouseWheel(MouseWheelEventArgs& args) {};
	virtual void OnResize(ResizeEventArgs& args) { _width = args.Width; _height = args.Height; };
	virtual void OnWindowDestroy() {};

public:
	Game(const std::wstring& name, int width, int height, bool vSync);
	virtual ~Game() = default;
	virtual bool Initialize();
	virtual bool LoadContent() = 0;
	virtual bool UnloadContent() = 0;
	virtual void Destroy();
};
