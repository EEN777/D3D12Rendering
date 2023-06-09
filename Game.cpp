#include "pch.h"
#include "Game.h"
#include "Application.h"
#include "Window.h"

Game::Game(const std::wstring& name, int width, int height, bool vSync) :
    _name{ name }, _width{ width }, _height{ height }, _vSync{ vSync } 
{
}

bool Game::Initialize()
{
    _window = Application::Get().CreateRenderWindow(_name, _width, _height, _vSync);
    _window->RegisterCallbacks(shared_from_this());
    _window->Show();

    return true;
}

void Game::Destroy()
{
    Application::Get().DestroyWindow(_window);
    _window.reset();
}
