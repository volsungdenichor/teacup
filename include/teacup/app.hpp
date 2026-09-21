#pragma once

#include <type_traits>
#include <utility>

#include "teacup/command.hpp"
#include "teacup/program.hpp"

namespace teacup
{

template <typename Model>
struct update_result
{
    Model model{};
    cmd command{};

    update_result() = default;

    update_result(Model m)  // NOLINT(google-explicit-constructor): `return model;`
        : model{ std::move(m) }
    {
    }

    update_result(Model m, cmd c) : model{ std::move(m) }, command{ std::move(c) } { }
};

template <typename Model>
update_result<std::decay_t<Model>> next(Model&& model)
{
    return update_result<std::decay_t<Model>>{ std::forward<Model>(model) };
}

template <typename Model>
update_result<std::decay_t<Model>> next(Model&& model, cmd command)
{
    return update_result<std::decay_t<Model>>{ std::forward<Model>(model), std::move(command) };
}

namespace detail
{

template <typename T>
using decay_t = std::decay_t<T>;

template <typename Init, typename Model>
auto call_init(Init& init, const Model& model, int) -> decltype(cmd{ init(model) })
{
    return init(model);
}

template <typename Init, typename Model>
cmd call_init(Init& init, const Model&, long)
{
    return init();
}

struct no_init
{
    cmd operator()() const { return cmd::none(); }
};

}  // namespace detail
template <typename Model, typename Init, typename Update, typename View>
class app final : public detail::app_base
{
public:
    app(Model model, Init init, Update update, View view)
        : model_{ std::move(model) }
        , init_{ std::move(init) }
        , update_{ std::move(update) }
        , view_{ std::move(view) }
    {
    }

    [[nodiscard]] const Model& model() const { return model_; }

    cmd init() override { return detail::call_init(init_, model_, 0); }

    cmd update(const message& msg) override
    {
        update_result<Model> result = update_(model_, msg);
        model_ = std::move(result.model);
        return std::move(result.command);
    }

    void view(canvas target) const override { view_(model_, target); }

private:
    Model model_;
    Init init_;
    Update update_;
    View view_;
};

template <typename Model, typename Init, typename Update, typename View>
app<detail::decay_t<Model>, detail::decay_t<Init>, detail::decay_t<Update>, detail::decay_t<View>> make_app(
    Model&& model, Init&& init, Update&& update, View&& view)
{
    return { std::forward<Model>(model), std::forward<Init>(init), std::forward<Update>(update), std::forward<View>(view) };
}

template <typename Model, typename Update, typename View>
app<detail::decay_t<Model>, detail::no_init, detail::decay_t<Update>, detail::decay_t<View>> make_app(
    Model&& model, Update&& update, View&& view)
{
    return { std::forward<Model>(model), detail::no_init{}, std::forward<Update>(update), std::forward<View>(view) };
}

}  // namespace teacup
