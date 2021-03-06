/*
    Aseba - an event-based framework for distributed robot control
    Created by Stéphane Magnenat <stephane at magnenat dot net> (http://stephane.magnenat.net)
    with contributions from the community.
    Copyright (C) 2007--2018 the authors, see authors.txt for details.

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU Lesser General Public License as published
    by the Free Software Foundation, version 3 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef ASEBA_HTTP_HANDLER
#define ASEBA_HTTP_HANDLER

#include <cassert>
#include <string>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"

namespace Aseba {
namespace Http {
    class HttpInterface;  // forward declaration

    /**
     * Base class for HTTP handlers. Handlers need to be able to check if they are responsible for
     * an incoming request, and the need to handle it in some way if a previous call to the checking
     * function was successful.
     *
     * This base class is intended to be used with multiple inheritance, so make sure to always
     * virtually inheriting from it.
     */
    class HttpHandler {
    public:
        HttpHandler() = default;
        virtual ~HttpHandler() = default;

        virtual bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const = 0;
        virtual void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) = 0;
    };

    class InterfaceHttpHandler : public virtual HttpHandler {
    public:
        InterfaceHttpHandler(HttpInterface* interface_) : interface(interface_) {}

        virtual const HttpInterface* getInterface() const {
            return interface;
        }

    protected:
        virtual HttpInterface* getInterface() {
            return interface;
        }

    private:
        HttpInterface* interface;
    };

    class WildcardHttpHandler : public virtual HttpHandler {
    public:
        WildcardHttpHandler() = default;
        ~WildcardHttpHandler() override = default;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override {
            return true;
        }
    };

    class HierarchicalHttpHandler : public virtual HttpHandler {
    public:
        HierarchicalHttpHandler() = default;

        ~HierarchicalHttpHandler() override {
            for(int i = 0; i < getNumSubhandlers(); i++) {
                delete subhandlers[i];
            }
        }

        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override {
            for(int i = 0; i < getNumSubhandlers(); i++) {
                if(subhandlers[i]->checkIfResponsible(request, tokens)) {
                    subhandlers[i]->handleRequest(request, tokens);
                    return;
                }
            }

            request->respond().setStatus(HttpResponse::HTTP_STATUS_NOT_FOUND);
        }

        virtual void addSubhandler(HttpHandler* subhandler) {
            subhandlers.push_back(subhandler);
        }
        virtual int getNumSubhandlers() const {
            return (int)subhandlers.size();
        }

    private:
        std::vector<HttpHandler*> subhandlers;
    };

    class RootHttpHandler : public WildcardHttpHandler, public HierarchicalHttpHandler {
    public:
        RootHttpHandler() = default;
        ~RootHttpHandler() override = default;
    };

    class TokenHttpHandler : public virtual HttpHandler {
    public:
        TokenHttpHandler() = default;
        ~TokenHttpHandler() override = default;

        bool checkIfResponsible(HttpRequest* request, const std::vector<std::string>& tokens) const override {
            if(tokens.empty()) {
                return false;
            }

            for(int i = 0; i < getNumTokens(); i++) {
                if(this->tokens[i] == tokens[0]) {
                    return true;
                }
            }

            return false;
        }

        virtual void addToken(const std::string& token) {
            tokens.push_back(token);
        }
        virtual const std::vector<std::string>& getTokens() const {
            return tokens;
        }
        virtual int getNumTokens() const {
            return (int)tokens.size();
        }

    private:
        std::vector<std::string> tokens;
    };

    class HierarchicalTokenHttpHandler : public HierarchicalHttpHandler, public TokenHttpHandler {
    public:
        HierarchicalTokenHttpHandler() = default;
        ~HierarchicalTokenHttpHandler() override = default;

        void handleRequest(HttpRequest* request, const std::vector<std::string>& tokens) override {
            assert(!tokens.empty());
            std::vector<std::string> newTokens(tokens.begin() + 1,
                                               tokens.end());  // eat first token

            HierarchicalHttpHandler::handleRequest(request, newTokens);
        }
    };
}  // namespace Http
}  // namespace Aseba

#endif
