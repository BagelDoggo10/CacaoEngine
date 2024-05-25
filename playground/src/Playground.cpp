#include "Cacao.hpp"

#include <chrono>

#define ICOSPHERE_COUNT 9
#define ICOSPHERE_RANGE 5

class PlaygroundApp {
  public:
	static PlaygroundApp* GetInstance() {
		//Do we have an instance yet?
		if(!instanceExists || instance == NULL) {
			//Create instance
			instance = new PlaygroundApp();
			instanceExists = true;
		}

		return instance;
	}

	void Launch();

	void Cleanup() {
		shader->Release();
		mesh->Release();
		sky->Release();
		delete mat;
		delete this;
	}

	Cacao::AssetHandle<Cacao::Skybox>& GetSky() {
		return sky;
	}

  private:
	static PlaygroundApp* instance;
	static bool instanceExists;

	Cacao::Material* mat;

	Cacao::AssetHandle<Cacao::Shader> shader;
	Cacao::AssetHandle<Cacao::Mesh> mesh;
	Cacao::AssetHandle<Cacao::Skybox> sky;
	Cacao::AssetHandle<Cacao::Sound> bgm;

	std::shared_ptr<Cacao::Entity> cameraManager;
	std::shared_ptr<Cacao::AudioPlayer> audioPlayer;
	std::vector<std::shared_ptr<Cacao::Entity>> icospheres;
};

class SussyScript final : public Cacao::Script {
  public:
	void OnActivate() override {
		Cacao::Logging::ClientLog("I'm awake!");
	}
	void OnDeactivate() override {
		Cacao::Logging::ClientLog("I'm asleep!");
	}
	void OnTick(double timestep) override {
		Cacao::World& world = Cacao::WorldManager::GetInstance()->GetActiveWorld();
		Cacao::PerspectiveCamera* cam = static_cast<Cacao::PerspectiveCamera*>(world.cam);
		glm::vec3 camRotChange = glm::vec3(0.0f);
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_J)) {
			camRotChange.y -= 0.5f;
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_K)) {
			camRotChange.y += 0.5f;
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_Y)) {
			camRotChange.x += 0.5f;
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_U)) {
			camRotChange.x -= 0.5f;
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_X)) {
			camRotChange.z += 0.5f;
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_C)) {
			camRotChange.z -= 0.5f;
		}

		currentRot = cam->GetRotation();
		currentRot += camRotChange;

		if(currentRot.x < -89.99) {
			currentRot.x = -89.99f;
		}
		if(currentRot.x > 89.99) {
			currentRot.x = 89.99f;
		}
		if(currentRot.y < 0) {
			currentRot.y = 360.0f;
		}
		if(currentRot.y > 360) {
			currentRot.y = 0.0f;
		}
		if(currentRot.z < 0) {
			currentRot.z = 360.0f;
		}
		if(currentRot.z > 360) {
			currentRot.z = 0.0f;
		}

		glm::vec3 posChange = glm::vec3(0.0f);
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_W)) {
			posChange += cam->GetFrontVector() * 5.0f * float(timestep);
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_S)) {
			posChange -= cam->GetFrontVector() * 5.0f * float(timestep);
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_D)) {
			posChange += cam->GetRightVector() * 5.0f * float(timestep);
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_A)) {
			posChange -= cam->GetRightVector() * 5.0f * float(timestep);
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_E)) {
			posChange += cam->GetUpVector() * 5.0f * float(timestep);
		}
		if(Cacao::Input::GetInstance()->IsKeyPressed(CACAO_KEY_Q)) {
			posChange -= cam->GetUpVector() * 5.0f * float(timestep);
		}

		currentPos = cam->GetPosition() + posChange;

		cam->SetRotation(Orientation(currentRot));
		cam->SetPosition(currentPos);
	}

  private:
	glm::vec3 currentRot, currentPos;
};

PlaygroundApp* PlaygroundApp::instance = nullptr;
bool PlaygroundApp::instanceExists = false;

void PlaygroundApp::Launch() {
	Cacao::WorldManager::GetInstance()->CreateWorld<Cacao::PerspectiveCamera>("Playground");
	Cacao::WorldManager::GetInstance()->SetActiveWorld("Playground");
	Cacao::World& world = Cacao::WorldManager::GetInstance()->GetWorld("Playground");

	std::shared_ptr<SussyScript> ss = std::make_shared<SussyScript>();
	ss->SetActive(true);

	std::future<Cacao::AssetHandle<Cacao::Shader>> shaderFuture = Cacao::AssetManager::GetInstance()->LoadShader("assets/shaders/color.shaderdef.yml");
	std::future<Cacao::AssetHandle<Cacao::Skybox>> skyFuture = Cacao::AssetManager::GetInstance()->LoadSkybox("assets/sky/sky.cubedef.yml");
	std::future<Cacao::AssetHandle<Cacao::Mesh>> meshFuture = Cacao::AssetManager::GetInstance()->LoadMesh("assets/models/icosphere.obj:Icosphere");
	std::future<Cacao::AssetHandle<Cacao::Sound>> bgmFuture = Cacao::AssetManager::GetInstance()->LoadSound("assets/audio/chords.mp3");

	shader = shaderFuture.get();
	mesh = meshFuture.get();
	sky = skyFuture.get();
	bgm = bgmFuture.get();

	mat = new Cacao::Material();
	mat->shader = shader.GetManagedAsset().get();

	audioPlayer = std::make_shared<Cacao::AudioPlayer>();
	audioPlayer->is3D = false;
	audioPlayer->isLooping = true;
	audioPlayer->gain = 1.0;
	audioPlayer->sound = bgm;

	cameraManager = std::make_shared<Cacao::Entity>("Camera Manager");
	cameraManager->active = true;
	cameraManager->components.push_back(ss);
	cameraManager->components.push_back(audioPlayer);
	world.topLevelEntities.push_back(cameraManager);

	std::random_device dev;
	std::mt19937 rng(dev());
	std::uniform_int_distribution<std::mt19937::result_type> dist(-ICOSPHERE_RANGE, ICOSPHERE_RANGE);

	for(int i = 0; i < ICOSPHERE_COUNT; i++) {
		std::stringstream ss;
		ss << "Icosphere #" << (i + 1);
		Cacao::Logging::ClientLog(ss.str());
		icospheres.push_back(std::make_shared<Cacao::Entity>(ss.str()));
		std::shared_ptr<Cacao::MeshComponent> mc = std::make_shared<Cacao::MeshComponent>();
		mc->SetActive(true);
		mc->mesh = mesh.GetManagedAsset().get();
		mc->mat = mat;
		icospheres[i]->components.push_back(mc);
		int x = dist(rng), y = dist(rng), z = dist(rng);
		icospheres[i]->transform.SetPosition({x, y, z});
		icospheres[i]->active = true;
		world.topLevelEntities.push_back(icospheres[i]);
	}

	world.skybox = sky.GetManagedAsset().get();

	audioPlayer->Play();
}

extern "C" {
void _CacaoLaunch() {
	PlaygroundApp::GetInstance()->Launch();
}

void _CacaoExiting() {
	PlaygroundApp::GetInstance()->Cleanup();
}
}