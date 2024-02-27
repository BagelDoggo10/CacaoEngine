#include "Control/DynTickController.hpp"

#include "Core/Log.hpp"
#include "Core/Engine.hpp"
#include "Utilities/MiscUtils.hpp"
#include "World/WorldManager.hpp"
#include "Scripts/Script.hpp"

#include <typeinfo>

namespace Cacao {
	//Required static variable initialization
	DynTickController* DynTickController::instance = nullptr;
	bool DynTickController::instanceExists = false;

	//Singleton accessor
	DynTickController* DynTickController::GetInstance() {
		//Do we have an instance yet?
		if(!instanceExists || instance == NULL){
			//Create instance
			instance = new DynTickController();
			instanceExists = true;
			instance->isRunning = false;
		}

		return instance;
	}

	void DynTickController::Start(){
		if(isRunning) {
			Logging::EngineLog("Cannot start the already started dynamic tick controller!", LogLevel::Error);
			return;
		}
		isRunning = true;
		//Create thread to run controller
		thread = new std::jthread(BIND_MEMBER_FUNC(DynTickController::Run));
	}

	void DynTickController::Stop(){
		if(!isRunning) {
			Logging::EngineLog("Cannot stop the not started dynamic tick controller!", LogLevel::Error);
			return;
		}
		//Stop run thread
		thread->request_stop();
		thread->join();

		//Delete thread object
		delete thread;
		thread = nullptr;

		isRunning = false;
	}

	void DynTickController::Run(std::stop_token stopTkn) {
		//Run while we haven't been asked to stop
		while(!stopTkn.stop_requested()){
			//Get time at tick start and calculate ideal run time
			std::chrono::steady_clock::time_point tickStart = std::chrono::steady_clock::now();
			std::chrono::steady_clock::time_point idealStopTime = tickStart + std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::seconds(1)/Engine::GetInstance()->cfg.targetDynTPS);
			
			//Find all scripts that need to be run
			std::vector<std::reference_wrapper<Script>> scriptsToRun;
			World& activeWorld = WorldManager::GetInstance()->AccessWorld(WorldManager::GetInstance()->GetActiveWorld());
			BS::multi_future<void> fsFuture = Engine::GetInstance()->GetThreadPool().submit_loop<unsigned int>(0, activeWorld.worldTree.children.size(), [scriptsToRun, activeWorld](unsigned int index) {
				//Create script locator function for an entity
				auto scriptLocator = [scriptsToRun](TreeItem<Entity>& e) {
					//Sneaky recursive lambda trick
					auto impl = [scriptsToRun](TreeItem<Entity>& e, auto& implRef) mutable {
						//Stop if this component is inactive
						if(!e.val().active) return;

						//Check for script components
						for(Component& c : e.val().components){
							if(typeid(c) == typeid(Script&) && c.IsActive()) {
								//Cast to script and add to list
								Script& asScript = static_cast<Script&>(c);
								scriptsToRun.push_back(asScript);
							}
						}

						//Recurse through children
						for(TreeItem<Entity>& child : e.children){
							implRef(child, implRef);
						}
					};
					return impl(e, impl);
				};

				//Execute the script locator
				TreeItem<Entity>& ent = const_cast<TreeItem<Entity>&>(activeWorld.worldTree.children.at(index)); //We have to use const_cast because at() returns a const reference
				scriptLocator(ent);
			});
			//Wait for work to be completed
			fsFuture.wait();

			//Check elapsed time
			std::chrono::steady_clock::time_point tickEnd = std::chrono::steady_clock::now();
			
			//If we stopped before the ideal max time, wait until that point
			//Otherwise, run the next tick immediately
			if(tickEnd < idealStopTime) std::this_thread::sleep_for(idealStopTime - tickEnd);
		}
	}
}
