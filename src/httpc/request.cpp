#include "request.hpp"

#include "init_guard.hpp"

#include <curl/curl.h>

using namespace httpc;

size_t request::write_data(void *buffer, size_t size, size_t nmemb, void *userp){
	ASSERT(userp)
	auto r = reinterpret_cast<request*>(userp);

	auto s = utki::make_span(reinterpret_cast<uint8_t*>(buffer), nmemb);

	if(r->data_handler){
		return r->data_handler(s);
	}else{
		r->resp.body.reserve(r->resp.body.size() + s.size());
		r->resp.body.insert(r->resp.body.end(), s.begin(), s.end());
		return s.size();
	}
}

request::request(decltype(completed_handler)&& ch) :
		CURL_handle(curl_easy_init()),
		completed_handler(std::move(ch))
{
	// curl_easy_setopt(this->handle, CURLOPT_VERBOSE, 1);

	curl_easy_setopt(this->CURL_handle, CURLOPT_WRITEFUNCTION, &write_data);
	curl_easy_setopt(this->CURL_handle, CURLOPT_WRITEDATA, this);
}

request::~request()noexcept{
	ASSERT(this->is_idle)
	curl_easy_cleanup(this->CURL_handle);
	this->free_headers();
}

void request::start(){
	if(!this->is_idle){
		throw std::logic_error("request is already running");
	}

	this->is_idle = false; // this should go before start_request()
	init_guard::inst().start_request(this->shared_from_this());
}

bool request::cancel()noexcept{
	return init_guard::inst().cancel_request(*this);
}

void request::set_url(const std::string& url){
	if(!this->is_idle){
		throw std::logic_error("could not set request URL: request is running");
	}
	curl_easy_setopt(this->CURL_handle, CURLOPT_URL, url.c_str());
}

void request::set_data_handler(decltype(data_handler)&& handler){
	if(!this->is_idle){
		throw std::logic_error("could not set request data handler: request is running");
	}
	this->data_handler = std::move(handler);
}

void request::free_headers()noexcept{
	ASSERT(this->is_idle)
	if(this->curl_slist_handle){
		curl_slist_free_all(reinterpret_cast<curl_slist*>(this->curl_slist_handle));
	}
}

void request::set_headers(const std::map<std::string, std::string>& name_value, utki::span<const std::string> name){
	curl_easy_setopt(this->CURL_handle, CURLOPT_HTTPHEADER, nullptr);
	this->free_headers();

	this->curl_slist_handle = nullptr; // curl_slist_append() creates a new curl_slist object when nullptr is passed as firsdt argument

	for(auto& nv : name_value){
		std::stringstream ss;
		ss << nv.first << ":";
		if(!nv.second.empty()){
			ss << " " << nv.second;
		}
		auto res = curl_slist_append(reinterpret_cast<curl_slist*>(this->curl_slist_handle), ss.str().c_str());
		if(res){
			this->curl_slist_handle = res;
		}
	}

	for(auto& n : name){
		std::stringstream ss;
		ss << n << ";";
		auto res = curl_slist_append(reinterpret_cast<curl_slist*>(this->curl_slist_handle), ss.str().c_str());
		if(res){
			this->curl_slist_handle = res;
		}
	}

	curl_easy_setopt(this->CURL_handle, CURLOPT_HTTPHEADER, this->curl_slist_handle);
}
