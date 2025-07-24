#pragma once
#include "KamataEngine.h"
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>


class GameScene {
public:
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	KamataEngine::DirectXCommon* dxCommon_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	KamataEngine::Audio* audio_ = nullptr;

	// カメラ
	KamataEngine::Camera camera_;

	
};
