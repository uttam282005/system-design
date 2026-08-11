#include <bits/stdc++.h>
using namespace std;

class HttpRequest {
private:
  string url;
  string method;
  map<string, string> headers;
  map<string, string> queryParams;
  string body;
  int timeout;

  // Private constructor
  HttpRequest(const string &url, const string &method,
              const map<string, string> &headers,
              const map<string, string> &queryParams, const string &body,
              int timeout)
      : url(url), method(method), headers(headers), queryParams(queryParams),
        body(body), timeout(timeout) {}

public:
  string getUrl() const { return url; }
  string getMethod() const { return method; }

  void print() const {
    cout << "HttpRequest{url='" << url << "', method='" << method
         << "', headers=" << headers.size()
         << ", queryParams=" << queryParams.size() << ", body='" << body
         << "', timeout=" << timeout << "}" << endl;
  }

  class Builder {
  private:
    string url;
    string method = "GET";
    map<string, string> headers;
    map<string, string> queryParams;
    string body;
    int timeout = 30000;

  public:
    explicit Builder(const string &url) : url(url) {}

    Builder &setMethod(const string &m) {
      method = m;
      return *this;
    }

    Builder &addHeader(const string &key, const string &value) {
      headers[key] = value;
      return *this;
    }

    Builder &addQueryParam(const string &key, const string &value) {
      queryParams[key] = value;
      return *this;
    }

    Builder &setBody(const string &b) {
      body = b;
      return *this;
    }

    Builder &setTimeout(int t) {
      timeout = t;
      return *this;
    }

    HttpRequest build() const {
      return HttpRequest(url, method, headers, queryParams, body, timeout);
    }
  };
};

int main() {
  // Simple GET request
  HttpRequest get =
      HttpRequest::Builder("https://api.example.com/users").build();

  // POST with body and custom timeout
  HttpRequest post =
      HttpRequest::Builder("https://api.example.com/users")
          .setMethod("POST")
          .addHeader("Content-Type", "application/json")
          .setBody("{\"name\":\"Alice\",\"email\":\"alice@example.com\"}")
          .setTimeout(5000)
          .build();

  // Authenticated PUT with query parameters
  HttpRequest put = HttpRequest::Builder("https://api.example.com/config")
                        .setMethod("PUT")
                        .addHeader("Authorization", "Bearer token123")
                        .addHeader("Content-Type", "application/json")
                        .addQueryParam("env", "production")
                        .addQueryParam("version", "2")
                        .setBody("{\"feature_flag\":true}")
                        .setTimeout(10000)
                        .build();

  get.print();
  post.print();
  put.print();

  return 0;
}

namespace DirectorPatter {
class HttpRequestDirector {
public:
  HttpRequest buildSimpleGet(const string &url) {
    return HttpRequest::Builder(url).setMethod("GET").setTimeout(30000).build();
  }

  HttpRequest buildAuthenticatedPost(const string &url, const string &token,
                                     const string &body) {
    return HttpRequest::Builder(url)
        .setMethod("POST")
        .addHeader("Authorization", "Bearer " + token)
        .addHeader("Content-Type", "application/json")
        .setBody(body)
        .setTimeout(10000)
        .build();
  }

  HttpRequest buildInternalServiceCall(const string &url) {
    return HttpRequest::Builder(url)
        .setMethod("GET")
        .addHeader("X-Internal-Service", "true")
        .setTimeout(5000)
        .build();
  }
};

// Usage
int main() {
  HttpRequestDirector director;

  auto get = director.buildSimpleGet("https://api.example.com/users");
  auto post = director.buildAuthenticatedPost("https://api.example.com/orders",
                                              "token123", R"({"item":"book"})");
  auto internal =
      director.buildInternalServiceCall("https://internal.service/health");

  get.print();
  post.print();
  internal.print();

  return 0;
}
} // namespace DirectorPatter
