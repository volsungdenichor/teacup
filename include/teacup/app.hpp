#pragma once

#include <type_traits>
#include <utility>

#include "teacup/command.hpp"
#include "teacup/program.hpp"

namespace teacup
{

template <typename Model>
struct UpdateResult
{
    Model model{};
    Cmd command{};

    UpdateResult() = default;

    UpdateResult(Model m)  // NOLINT(google-explicit-constructor): `return model;`
        : model{ std::move(m) }
    {
    }

    UpdateResult(Model m, Cmd c) : model{ std::move(m) }, command{ std::move(c) } { }
};

template <typename Model>
UpdateResult<std::decay_t<Model>> next(Model&& model)
{
    return UpdateResult<std::decay_t<Model>>{ std::forward<Model>(model) };
}

template <typename Model>
UpdateResult<std::decay_t<Model>> next(Model&& model, Cmd command)
{
    return UpdateResult<std::decay_t<Model>>{ std::forward<Model>(model), std::move(command) };
}

namespace detail
{

template <typename T>
using decay_t = std::decay_t<T>;

template <typename Init, typename Model>
auto call_init(Init& init, const Model& model, int) -> decltype(Cmd{ init(model) })
{
    return init(model);
}

template <typename Init, typename Model>
Cmd call_init(Init& init, const Model&, long)
{
    return init();
}

struct no_init
{
    Cmd operator()() const { return Cmd::none(); }
};

}  // namespace detail
template <typename Model, typename Init, typename Update, typename View>
class App final : public detail::AppBase
{
public:
    App(Model model, Init init, Update update, View view)
        : model_{ std::move(model) }
        , init_{ std::move(init) }
        , update_{ std::move(update) }
        , view_{ std::move(view) }
    {
    }

    [[nodiscard]] const Model& model() const { return model_; }

    Cmd init() override { return detail::call_init(init_, model_, 0); }

    Cmd update(const Message& msg) override
    {
        UpdateResult<Model> result = update_(model_, msg);
        model_ = std::move(result.model);
        return std::move(result.command);
    }

    void view(Canvas target) const override { view_(model_, target); }

private:
    Model model_;
    Init init_;
    Update update_;
    View view_;
};

template <typename Model, typename Init, typename Update, typename View>
App<detail::decay_t<Model>, detail::decay_t<Init>, detail::decay_t<Update>, detail::decay_t<View>> make_app(
    Model&& model, Init&& init, Update&& update, View&& view)
{
    return { std::forward<Model>(model), std::forward<Init>(init), std::forward<Update>(update), std::forward<View>(view) };
}

template <typename Model, typename Update, typename View>
App<detail::decay_t<Model>, detail::no_init, detail::decay_t<Update>, detail::decay_t<View>> make_app(
    Model&& model, Update&& update, View&& view)
{
    return { std::forward<Model>(model), detail::no_init{}, std::forward<Update>(update), std::forward<View>(view) };
}

}  // namespace teacup
