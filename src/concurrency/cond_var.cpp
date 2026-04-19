#include "concurrency/cond_var.h"

namespace ufps
{
    auto CondVar::notify_one() -> void
    {
        _cv.notify_one();
    }

    auto CondVar::notify_all() -> void
    {
        _cv.notify_all();
    }

}