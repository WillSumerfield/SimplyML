#include <algorithm>
#include <optional>
#include <string>
#include <tuple>
#include <vector>

#include <nanobind/ndarray.h>
#include <nanobind/stl/optional.h>
#include <nanobind/stl/string.h>
#include <nanobind/stl/tuple.h>
#include <nanobind/stl/vector.h>

#include "simplyml/ml/cart_pendulum_view.hpp"
#include "simplyml/ml/image_view.hpp"
#include "simplyml/ml/network_view.hpp"
#include "simplyml/ml/training_stats_card.hpp"
#include "widget_refs.hpp"

using namespace nb::literals;

namespace
{

using Ints    = nb::ndarray<int const, nb::ndim<1>, nb::c_contig, nb::device::cpu>;
using Floats  = nb::ndarray<float const, nb::ndim<1>, nb::c_contig, nb::device::cpu>;
using Rgba    = nb::ndarray<std::uint8_t const, nb::shape<-1, -1, 4>, nb::c_contig, nb::device::cpu>;
using Points  = nb::ndarray<float const, nb::shape<-1, 2>, nb::c_contig, nb::device::cpu>;
using Points3 = nb::ndarray<float const, nb::shape<-1, -1, 2>, nb::c_contig, nb::device::cpu>;

std::vector<sf::Vector2f> points(float const* p, std::size_t n)
{
    std::vector<sf::Vector2f> out(n);
    for (std::size_t i = 0; i < n; ++i) {
        out[i] = {p[2 * i], p[2 * i + 1]};
    }
    return out;
}

} // namespace

void bindMl(nb::module_& m, nb::class_<ContainerRef, WidgetRef>& container)
{
    nb::class_<NetworkViewRef, WidgetRef>(m, "NetworkView", "Network drawing, layered or placed (see `set_graph`).")
        .def("_set_graph", [](NetworkViewRef const& r, Ints layers, Ints src, Ints dst, std::optional<Floats> values,
                              std::optional<Floats> edgeValues, std::optional<std::vector<std::string>> labels) {
            std::size_t const n = layers.shape(0), e = src.shape(0);
            if (dst.shape(0) != e || (values && values->shape(0) != n) || (edgeValues && edgeValues->shape(0) != e)
                || (labels && labels->size() != n)) {
                throw nb::value_error("set_graph: array lengths don't match");
            }
            sml::LayeredGraph g;
            g.nodes.resize(n);
            for (std::size_t i = 0; i < n; ++i) {
                g.nodes[i].layer = layers(i);
                g.nodes[i].value = values ? (*values)(i) : 0.0f;
                if (labels) {
                    g.nodes[i].label = std::move((*labels)[i]);
                }
            }
            g.edges.resize(e);
            for (std::size_t i = 0; i < e; ++i) {
                g.edges[i] = {src(i), dst(i), edgeValues ? (*edgeValues)(i) : 0.0f};
            }
            as<sml::NetworkView>(r, "network view").setGraph(std::move(g)); // thread-safe; no Ui lock
        }, "layers"_a, "src"_a, "dst"_a, "values"_a.none(), "edge_values"_a.none(), "labels"_a.none())
        .def("_set_placed_graph", [](NetworkViewRef const& r, Points positions, Ints src, Ints dst,
                                     std::optional<Floats> values, std::optional<Floats> edgeValues,
                                     std::optional<std::vector<std::string>> labels) {
            std::size_t const n = positions.shape(0), e = src.shape(0);
            if (dst.shape(0) != e || (values && values->shape(0) != n) || (edgeValues && edgeValues->shape(0) != e)
                || (labels && labels->size() != n)) {
                throw nb::value_error("set_graph: array lengths don't match");
            }
            sml::PlacedGraph g;
            g.nodes.resize(n);
            for (std::size_t i = 0; i < n; ++i) {
                g.nodes[i].position = {positions(i, 0), positions(i, 1)};
                g.nodes[i].value    = values ? (*values)(i) : 0.0f;
                if (labels) {
                    g.nodes[i].label = std::move((*labels)[i]);
                }
            }
            g.edges.resize(e);
            for (std::size_t i = 0; i < e; ++i) {
                g.edges[i] = {src(i), dst(i), edgeValues ? (*edgeValues)(i) : 0.0f};
            }
            as<sml::NetworkView>(r, "network view").setGraph(std::move(g)); // thread-safe; no Ui lock
        }, "positions"_a, "src"_a, "dst"_a, "values"_a.none(), "edge_values"_a.none(), "labels"_a.none());

    nb::class_<ImageViewRef, WidgetRef>(m, "ImageView", "Image panel (see `set_image`).")
        .def("_set_image", [](ImageViewRef const& r, Rgba img) {
            auto const h = static_cast<unsigned>(img.shape(0)), w = static_cast<unsigned>(img.shape(1));
            sml::RgbaImage out{w, h, {img.data(), img.data() + std::size_t{w} * h * 4}};
            as<sml::ImageView>(r, "image view").setImage(std::move(out)); // thread-safe; no Ui lock
        }, "rgba"_a);

    nb::class_<CartPendulumRef, WidgetRef>(m, "CartPendulumView", "Cart-pendulum scene (see `set_state`).")
        .def("_set_state", [](CartPendulumRef const& r, float x, float y, Points joints, float push) {
            sml::LinkChainState s{{x, y}, points(joints.data(), joints.shape(0)), push};
            as<sml::CartPendulumView>(r, "cart pendulum view").setState(std::move(s));
        }, "x"_a, "y"_a, "joints"_a, "push"_a)
        .def("_set_ghosts", [](CartPendulumRef const& r, Points bases, Points3 joints) {
            if (bases.shape(0) != joints.shape(0)) {
                throw nb::value_error("set_ghosts: bases and joints differ in length");
            }
            std::size_t const g = bases.shape(0), n = joints.shape(1);
            std::vector<sml::LinkChainState> out(g);
            for (std::size_t i = 0; i < g; ++i) {
                out[i].base   = {bases(i, 0), bases(i, 1)};
                out[i].joints = points(joints.data() + i * n * 2, n);
            }
            as<sml::CartPendulumView>(r, "cart pendulum view").setGhosts(std::move(out));
        }, "bases"_a, "joints"_a);

    container
        .def("network_view", [](ContainerRef const& r, std::string title, nb::handle color, float edgeScale,
                                bool footer, float maxZoom, bool vertical, nb::kwargs kw) {
            auto o = addTo<sml::NetworkView>(r, kw, std::move(title), toColor(color, themeOf(r)));
            UiLock l{*r.app};
            as<sml::NetworkView>(nb::cast<WidgetRef const&>(o), "network view")
                .setEdgeScale(edgeScale).setFooter(footer).setMaxZoom(maxZoom).setVertical(vertical);
            return o;
        }, "title"_a = "", "color"_a = nb::none(), "edge_scale"_a = 20.0f, "footer"_a = true, "max_zoom"_a = 1.5f,
            "vertical"_a = false, "kw"_a,
            "Network drawing: columns per layer (rows when vertical), or nodes where a PlacedGraph puts them; node fill = |value|, edge width = |value| * edge_scale px; "
            "green positive, red negative. Feed it with `set_graph` (or `mlp_graph(...)`).")
        .def("image_view", [](ContainerRef const& r, std::string title, nb::handle color, nb::kwargs kw) {
            return addTo<sml::ImageView>(r, kw, std::move(title), toColor(color, themeOf(r)));
        }, "title"_a = "", "color"_a = nb::none(), "kw"_a,
            "Image panel: fitted with aspect kept, sharp pixels. Feed it with `set_image`.")
        .def("cart_pendulum", [](ContainerRef const& r, std::string title, nb::handle color,
                                 std::optional<std::tuple<float, float, float>> rail,
                                 std::optional<std::tuple<float, float, float, float>> world, bool ruler,
                                 int ghostAlpha, float pushScale, nb::kwargs kw) {
            auto o = addTo<sml::CartPendulumView>(r, kw, std::move(title), toColor(color, themeOf(r)));
            UiLock l{*r.app};
            auto& v = as<sml::CartPendulumView>(nb::cast<WidgetRef const&>(o), "cart pendulum view");
            if (rail) v.setRail(std::get<0>(*rail), std::get<1>(*rail), std::get<2>(*rail));
            if (world) v.setWorld({{std::get<0>(*world), std::get<1>(*world)}, {std::get<2>(*world), std::get<3>(*world)}});
            v.setRuler(ruler).setGhostAlpha(static_cast<std::uint8_t>(std::clamp(ghostAlpha, 0, 255))).setPushScale(pushScale);
            return o;
        }, "title"_a = "", "color"_a = nb::none(), "rail"_a = nb::none(), "world"_a = nb::none(), "ruler"_a = true,
            "ghost_alpha"_a = 50, "push_scale"_a = 6.6f, "kw"_a,
            "Cart-pendulum scene in world units (y down). `rail` = (from, to, y) of the base's travel; "
            "`world` = (x, y, w, h) shown, default around the rail.")
        .def("training_stats", [](ContainerRef const& r, std::string title, nb::handle color, std::string prefix,
                                  std::string label, std::string scoreLabel, std::string scoreFmt, nb::kwargs kw) {
            if (!kw.contains("size") && !kw.contains("weight")) {
                kw["fit"] = true;
            }
            auto const fmt = toFormat(scoreFmt);
            auto o = addTo<sml::TrainingStatsCard>(r, kw, std::move(title), toColor(color, themeOf(r)), prefix, std::move(label));
            UiLock l{*r.app};
            as<sml::TrainingStatsCard>(nb::cast<WidgetRef const&>(o), "training stats card").bestScore()
                .setLabel(std::move(scoreLabel)).setFormat(fmt);
            return o;
        }, "title"_a = "Iteration", "color"_a = nb::none(), "prefix"_a = "", "label"_a = "",
            "score_label"_a = "Best score", "score_fmt"_a = ".4", "kw"_a,
            "Stat card for `store.push_stats`: iteration, best score, simulated and real training time; "
            "add more rows with its methods.");
}
