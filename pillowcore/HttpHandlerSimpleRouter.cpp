#include "HttpHandlerSimpleRouter.h"
#include "HttpRequest.h"
#include <QtCore/QPointer>
#include <QtCore/QRegExp>
using namespace Pillow;

namespace Pillow
{
	struct Route
	{
		QRegExp regExp;
		
		virtual bool invoke(Pillow::HttpRequest* request) = 0;
	};
	
	struct HandlerRoute : public Route
	{
		QPointer<Pillow::HttpHandler> handler;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			if (!handler) return false;
			return handler->handleRequest(request);
		}
	};
	
	struct QObjectMetaCallRoute : public Route
	{
		QPointer<QObject> object;
		const char* member;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			if (!object) return false;
			return QMetaObject::invokeMethod(object, member, Q_ARG(Pillow::HttpRequest*, request));
		}
	};
	
	struct StaticRoute : public Route
	{
		int statusCode;
		Pillow::HttpHeaderCollection headers;
		QByteArray content;
		
		virtual bool invoke(Pillow::HttpRequest *request)
		{
			request->writeResponse(statusCode, headers, content);
			return true;
		}		
	};

	//
	// HttpHandlerSimpleRouterPrivate
	//

	class HttpHandlerSimpleRouterPrivate
	{
	public:
		QList<Route*> routes;
	};
}

//
// HttpHandlerSimpleRouter
//

HttpHandlerSimpleRouter::HttpHandlerSimpleRouter(QObject* parent /* = 0 */)
	: Pillow::HttpHandler(parent), d_ptr(new HttpHandlerSimpleRouterPrivate)
{
}

HttpHandlerSimpleRouter::~HttpHandlerSimpleRouter()
{
	foreach (Route* route, d_ptr->routes)
		delete route;
	delete d_ptr;
}

void HttpHandlerSimpleRouter::addRoute(const QString &path, Pillow::HttpHandler *handler)
{
	HandlerRoute* route = new HandlerRoute();
	route->regExp = pathToRegExp(path);
	route->handler = handler;
	d_ptr->routes.append(route);
}

void HttpHandlerSimpleRouter::addRoute(const QString& path, QObject* object, const char* member)
{
	QObjectMetaCallRoute* route = new QObjectMetaCallRoute();
	route->regExp = pathToRegExp(path);
	route->object = object;
	route->member = member;
	d_ptr->routes.append(route);
}

void HttpHandlerSimpleRouter::addRoute(const QString& path, int statusCode, const Pillow::HttpHeaderCollection& headers, const QByteArray& content /*= QByteArray()*/)
{
	StaticRoute* route = new StaticRoute();
	route->regExp = pathToRegExp(path);
	route->statusCode = statusCode;
	route->headers = headers;
	route->content = content;
	d_ptr->routes.append(route);
}

QRegExp Pillow::HttpHandlerSimpleRouter::pathToRegExp(const QString &path)
{
	return QRegExp("^" + path + "$");
}

bool HttpHandlerSimpleRouter::handleRequest(Pillow::HttpRequest *request)
{
	foreach (Route* route, d_ptr->routes) 
	{
		if (route->regExp.indexIn(request->requestPath()) != -1)
		{
			route->invoke(request);
			return true;
		}
	}
	
	return false;
}
