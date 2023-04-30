#include "CitrusClient.h"
#include "Assert.h"
#include "Graphics/Window.h"
#include "Log.h"
#include "Utilities/Utilities.h"

namespace CitrusEngine {

    //Required definitions of static members
    CitrusClient* CitrusClient::instance = nullptr;
    EventManager* CitrusClient::eventManager = nullptr;

    CitrusClient::CitrusClient(){
        //Confirm we are initializing the first client
        Asserts::EngineAssert(instance != nullptr, "Client instance already exists!");

        //Set singleton instance
        instance = this;

        //Allow the app to run
        run = true;

        //Set up event consumers
        wceConsumer = new EventConsumer(BIND_MEMBER_FUNC(CitrusClient::Shutdown));
        clientFixedTickConsumer = new EventConsumer(BIND_MEMBER_FUNC(CitrusClient::FixedTickHandler));
        clientDynamicTickConsumer = new EventConsumer(BIND_MEMBER_FUNC(CitrusClient::DynamicTickHandler));

        //Set up event manager and subscribe consumers
        eventManager = new EventManager();
        eventManager->SubscribeConsumer("WindowClose", wceConsumer);
        eventManager->SubscribeConsumer("ClientFixedTick", clientDynamicTickConsumer);
        eventManager->SubscribeConsumer("ClientDynamicTick", clientDynamicTickConsumer);

        //Initialize input and utilities
        Input::Create();
        Utilities::Create();
    }

    //Base client does not need a destructor
    CitrusClient::~CitrusClient() {}

    void CitrusClient::Run(){
        //Create window
        Window::Create(id, 1280, 720);

        //Allow client to set up
        this->ClientOnStartup();

        double elapsed = 0;

        while(run){
            double oldElapsed = elapsed; 
            elapsed = Utilities::GetElapsedTime();

            ClientDynamicTickEvent tickEvent{elapsed - oldElapsed};
            eventManager->Dispatch(tickEvent);

            Window::Update();
        }

        //Prepare eventManager for freeing by unsubscribing all consumers;
        eventManager->Shutdown();

        Window::Destroy();
        
        //Free pointers
        delete eventManager;
        delete wceConsumer;
    }

    void CitrusClient::Shutdown(Event& e){
        //Allow client to shut down
        this->ClientOnShutdown();

        e.handled = true;
        run = false;
    }

    void CitrusClient::FixedTickHandler(Event& e){
        ClientOnFixedTick();
        e.handled = true;
    }

    void CitrusClient::DynamicTickHandler(Event& e){
        ClientDynamicTickEvent cdte = Event::EventTypeCast<ClientDynamicTickEvent>(e);
        ClientOnDynamicTick(cdte.timestep);
        e.handled = true;
    }
}