#pragma once
#include "KamataEngine.h"
#include <2d/Sprite.h>
#include <3d/Camera.h>
#include <3d/Model.h>
#include <3d/WorldTransform.h>

#include <map>
#include <string>
#include <vector>

class GameScene {

	struct ObjectData {
		std::string type;
		std::string name;
		struct Transform {
			KamataEngine::Vector3 translation;
			KamataEngine::Vector3 rotation;
			KamataEngine::Vector3 scaling;
		};
		Transform transform;
		std::string file_name;
	};

	struct LevelData {
		std::string name;
		std::vector<ObjectData> objects;
	};

public:
	~GameScene();
	void Initialize();
	void Update();
	void Draw();

private:
	// 初期化補助関数
	void SceneJson();

private:
	KamataEngine::DirectXCommon* dxCommon_ = nullptr;
	KamataEngine::Input* input_ = nullptr;
	KamataEngine::Audio* audio_ = nullptr;
	KamataEngine::Camera camera_;

	// レベルデータ
	LevelData* levelData = nullptr;
	// JSONファイルから読み込んだモデルデータ
	std::map<std::string, KamataEngine::Model*> models;
	// オブジェクトのワールドトランスフォーム
	std::vector<KamataEngine::WorldTransform*> objects;

	// Player
	KamataEngine::Model* playerModel_ = nullptr;
	KamataEngine::WorldTransform playerTransform_;// Playerのワールドトランスフォーム

	// Plane
	KamataEngine::Model* planeModel_ = nullptr;
	KamataEngine::WorldTransform planeTransform_;

};
