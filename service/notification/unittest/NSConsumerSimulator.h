//******************************************************************
//
// Copyright 2016 Samsung Electronics All Rights Reserved.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
//-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=

#ifndef _NS_CONSUMER_SIMULATOR_H_
#define _NS_CONSUMER_SIMULATOR_H_

#include <iostream>

#include "OCPlatform.h"
#include "OCApi.h"

class NSConsumerSimulator
{
private:
    std::function<void(const int&, const std::string&, const std::string&,
            const std::string&)> m_messageFunc;
    std::function<void(const int&, const int&)> m_syncFunc;

    std::shared_ptr<OC::OCResource> m_syncResource;
    std::shared_ptr<OC::OCResource> m_msgResource;
    std::shared_ptr<OC::OCResource> m_topicResource;

public:
    NSConsumerSimulator()
    : m_messageFunc(), m_syncFunc(),
      m_syncResource() { };
    ~NSConsumerSimulator() = default;

    NSConsumerSimulator(const NSConsumerSimulator &) = delete;
    NSConsumerSimulator & operator = (const NSConsumerSimulator &) = delete;

    NSConsumerSimulator(NSConsumerSimulator &&) = delete;
    NSConsumerSimulator & operator = (NSConsumerSimulator &&) = delete;

    void findProvider()
    {
        OC::OCPlatform::findResource("", std::string("/oic/res?rt=oic.r.notification"),
                OCConnectivityType::CT_DEFAULT,
                std::bind(&NSConsumerSimulator::findResultCallback, this, std::placeholders::_1),
                OC::QualityOfService::LowQos);
    }

    void syncToProvider(int & type, const int & id, const std::string & providerID)
    {
        if (m_syncResource == nullptr)
        {
            std::cout << "m_syncResource is null" << std::endl;
            return;
        }

        OC::OCRepresentation rep;
        rep.setValue("PROVIDER_ID", providerID);
        rep.setValue("MESSAGE_ID", id);
        rep.setValue("STATE", type);

        m_syncResource->post(rep, OC::QueryParamsMap(), &onPost, OC::QualityOfService::LowQos);
    }

    bool cancelObserves()
    {
        if(!msgResourceCancelObserve(OC::QualityOfService::HighQos) &&
                !syncResourceCancelObserve(OC::QualityOfService::HighQos))
            return true;
        return false;
    }

    void setCallback(std::function<void(const int&, const std::string&,
            const std::string&, const std::string&)> messageFunc,
            const std::function<void(const int&, const int&)> & syncFunc)
    {
        m_messageFunc = messageFunc;
        m_syncFunc = syncFunc;
    }

private:
    static void onPost(const OC::HeaderOptions &/*headerOption*/,
                const OC::OCRepresentation & /*rep*/ , const int eCode)
    {
        std::cout << __func__ << " result : " << eCode << std::endl;
    }
    void findResultCallback(std::shared_ptr<OC::OCResource> resource)
    {

        std::cout << __func__ << " " << resource->host() << std::endl;

        if(resource->uri() == "/notification")
        {
            resource->get(OC::QueryParamsMap(),
                    std::bind(&NSConsumerSimulator::onGet, this,
                            std::placeholders::_1, std::placeholders::_2, std::placeholders::_3,
                            resource), OC::QualityOfService::LowQos);
        }
    }
    void onGet(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation & rep , const int eCode,
            std::shared_ptr<OC::OCResource> resource)
    {
        std::cout << __func__ << " " << rep.getHost() << " result : " << eCode << std::endl;

        OC::QueryParamsMap map;
        map.insert(std::pair<std::string,std::string>(std::string("consumerid"),
                std::string("123456789012345678901234567890123456")));

        try
        {
            std::cout << "resourc : host " << resource->host() << std::endl;
            std::cout << "resourc : uri " << resource->uri() << std::endl;
            std::cout << " resource->connectivityType() "
                    <<  resource->connectivityType() << std::endl;
            std::cout << "resourc : getResourceInterfaces "
                    << resource->getResourceInterfaces()[0] << std::endl;
            std::cout << "resourc : getResourceTypes "
                    << resource->getResourceTypes()[0] << std::endl;

            std::vector<std::string> rts{"oic.r.notification"};

            m_msgResource
                = OC::OCPlatform::constructResourceObject(
                        std::string(resource->host()), std::string(resource->uri() + "/message"),
                        OCConnectivityType(resource->connectivityType()), true, rts,
                        std::vector<std::string>(resource->getResourceInterfaces()));

            m_msgResource->observe(OC::ObserveType::Observe, map,
                            std::bind(&NSConsumerSimulator::onObserve, this,
                                    std::placeholders::_1, std::placeholders::_2,
                                    std::placeholders::_3, std::placeholders::_4, resource),
                            OC::QualityOfService::LowQos);
        }
        catch(std::exception & e)
        {
            std::cout << "OC::ResoureInitException : " << e.what() << std::endl;
        }
        m_syncResource
            = OC::OCPlatform::constructResourceObject(resource->host(), resource->uri() + "/sync",
                    resource->connectivityType(), true, resource->getResourceTypes(),
                    resource->getResourceInterfaces());

        m_syncResource->observe(OC::ObserveType::Observe, map,
                std::bind(&NSConsumerSimulator::onObserve, this,
                        std::placeholders::_1, std::placeholders::_2,
                        std::placeholders::_3, std::placeholders::_4, resource),
                OC::QualityOfService::LowQos);


        m_topicResource
            = OC::OCPlatform::constructResourceObject(resource->host(), resource->uri() + "/topic",
                    resource->connectivityType(), true, resource->getResourceTypes(),
                    resource->getResourceInterfaces());

    }
    void onObserve(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation &rep , const int &eCode, const int &,
            std::shared_ptr<OC::OCResource> )
    {
        std::cout << __func__ << " " << rep.getHost() << " result : " << eCode;
        std::cout << " uri : " << rep.getUri() << std::endl;

        if (rep.getUri() == "/notification/message" && rep.hasAttribute("MESSAGE_ID")
                && rep.getValue<int>("MESSAGE_ID") != 1)
        {
            std::cout << "ID : " << rep.getValue<int>("ID") << std::endl;
            std::cout << "TITLE : " << rep.getValueToString("TITLE") << std::endl;
            std::cout << "CONTENT : " << rep.getValueToString("CONTENT") << std::endl;
            m_messageFunc(int(rep.getValue<int>("MESSAGE_ID")),
                          std::string(rep.getValueToString("TITLE")),
                          std::string(rep.getValueToString("CONTENT")),
                          std::string(rep.getValueToString("SOURCE")));

            if(rep.getValue<int>("MESSAGE_ID") == 3)
            {
                m_topicResource->get(OC::QueryParamsMap(),
                        std::bind(&NSConsumerSimulator::onTopicGet, this, std::placeholders::_1,
                                std::placeholders::_2, std::placeholders::_3, m_topicResource),
                                OC::QualityOfService::LowQos);
            }
        }
        else if (rep.getUri() == "/notification/sync")
        {
            std::cout << "else if (rep.getUri() == sync) " << std::endl;
            m_syncFunc(int(rep.getValue<int>("STATE")), int(rep.getValue<int>("ID")));
        }
    }

    void onTopicGet(const OC::HeaderOptions &/*headerOption*/,
            const OC::OCRepresentation & rep , const int eCode,
            std::shared_ptr<OC::OCResource> resource)
    {
        //TO-DO using this function.
        (void) rep;
        (void) eCode;
        (void) resource;
        std::cout << "onTopicGet()" << std::endl;
    }

    OCStackResult msgResourceCancelObserve(OC::QualityOfService qos)
    {
        return m_msgResource->cancelObserve(qos);
    }

    OCStackResult syncResourceCancelObserve(OC::QualityOfService qos)
    {
        return m_syncResource->cancelObserve(qos);
    }
};


#endif //_NS_CONSUMER_SIMULATOR_H_
