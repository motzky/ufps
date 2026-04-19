#pragma once

#include <format>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>

#include "utils/formatter.h"

namespace ufps
{
    class Thread
    {
    public:
        template <class F, class... Args>
        Thread(std::string_view name, F &&func, Args &&...args)
            : _name{name},
              _exception{},
              _thread{
                  [&, this]<class... A>(std::stop_token stop_token, F &&func, A &&...a)
                  {
                      try
                      {
                          func(stop_token, std::forward<A>(a)...);
                      }
                      catch (...)
                      {
                          _exception = std::current_exception();
                      }
                  },
                  std::forward<F>(func),
                  std::forward<Args>(args)...}

        {
        }

        auto request_stop() -> void
        {
            _thread.request_stop();
        }

        auto id() const -> std::thread::id
        {
            return _thread.get_id();
        }

        auto name() const -> std::string_view
        {
            return _name;
        }

        auto to_string() -> std::string
        {
            return std::format("thread: {} [{}]", _name, _thread.get_id());
        }

        auto exception() const -> std::exception_ptr
        {
            return _exception;
        }

    private:
        std::string _name;
        std::exception_ptr _exception;
        std::jthread _thread;
    };
}