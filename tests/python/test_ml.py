import numpy as np
import pytest

import simplyml


def test_mlp_graph_from_weights():
    w1 = np.arange(6, dtype=np.float32).reshape(3, 2)  # 2 -> 3
    w2 = np.ones((1, 3))
    g = simplyml.mlp_graph([w1, w2], activations=[[1, 2], [0.1, 0.2, 0.3], [0.5]], input_labels=["a", "b"],
                           output_labels=["y"])
    assert list(g.layers) == [0, 0, 1, 1, 1, 2]
    edges = np.asarray(g.edges)
    assert len(edges) == 9
    # edge (node 1 -> node 2+o) carries w1[o, 1]
    k = [i for i, (s, d) in enumerate(edges) if s == 1 and d == 4][0]
    assert g.edge_values[k] == w1[2, 1]
    assert list(g.values) == pytest.approx([1, 2, 0.1, 0.2, 0.3, 0.5])
    assert g.labels == ["a", "b", "", "", "", "y"]


def test_mlp_graph_from_torch():
    torch = pytest.importorskip("torch")
    model = torch.nn.Sequential(torch.nn.Linear(2, 4), torch.nn.Tanh(), torch.nn.Linear(4, 1))
    g = simplyml.mlp_graph(model, x=[0.5, -0.5])
    assert len(g.layers) == 7 and len(g.edges) == 12
    h = torch.tanh(model[0](torch.tensor([0.5, -0.5])))
    assert g.values[2:6] == pytest.approx(h.detach().numpy(), abs=1e-6)


def test_ml_widgets_and_stats():
    app = simplyml.App()
    row = app.ui.row()
    net = row.network_view("Net", color="white", edge_scale=2, id="net")
    assert isinstance(net, simplyml.NetworkView) and isinstance(app["net"], simplyml.NetworkView)
    net.set_graph(simplyml.mlp_graph([np.ones((3, 2)), np.ones((1, 3))]))
    net.set_graph(layers=[0, 1], edges=[(0, 1)], values=[0.5, -1], edge_values=[2.0])
    with pytest.raises(ValueError):
        net._set_graph(np.zeros(2, np.int32), np.zeros(1, np.int32), np.zeros(2, np.int32), None, None, None)

    net.set_graph(simplyml.PlacedGraph([(0, 0), (1, 2)], [(0, 1), (1, 1)], values=[0.5, -1], labels=["a", "b"]))
    with pytest.raises(ValueError):
        net._set_placed_graph(np.zeros((2, 2), np.float32), np.zeros(1, np.int32), np.zeros(2, np.int32), None, None,
                              None)

    class Fake:
        def __init__(self):
            self.calls = []

        def _set_graph(self, *a):
            self.calls.append(("layered", a))

        def _set_placed_graph(self, *a):
            self.calls.append(("placed", a))

    f = Fake()
    simplyml.NetworkView.set_graph(f, positions=[(0, 0), (3, 4)], edges=[(0, 1)])
    simplyml.NetworkView.set_graph(f, layers=[0, 1], edges=[(0, 1)])
    assert [kind for kind, _ in f.calls] == ["placed", "layered"]
    assert f.calls[0][1][0].shape == (2, 2) and f.calls[0][1][0].dtype == np.float32

    cart = row.cart_pendulum(rail=(220, 720, 225), world=(-25, -25, 990, 500), id="cart")
    assert isinstance(cart, simplyml.CartPendulumView)
    cart.set_state((470, 225), [(470, 325), (470, 425)], push=20)
    cart.set_ghosts(np.zeros((5, 2)), np.zeros((5, 2, 2)))
    with pytest.raises(ValueError):
        cart._set_ghosts(np.zeros((2, 2), np.float32), np.zeros((3, 1, 2), np.float32))

    card = row.training_stats(color="teal", prefix="t_")
    assert isinstance(card, simplyml.StatCard)
    card.gauge("Gravity", "t_gravity", 0, 2000)
    app.store.push_stats(3, best_score=1.5, sim_time=60, wall_time=2, prefix="t_", gravity=980)

    class Recorder:
        def __init__(self):
            self.calls = []

        def push(self, name, value, step):
            self.calls.append((name, value, step))

    r = Recorder()
    simplyml.MetricStore.push_stats(r, 3, best_score=1.5, prefix="t_", gravity=980)
    assert ("t_gravity", 980.0, 3.0) in r.calls and ("t_iteration", 3.0, 3.0) in r.calls and len(r.calls) == 5
