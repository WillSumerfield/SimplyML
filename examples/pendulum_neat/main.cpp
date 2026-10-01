// Pendulum-NEAT, rebuilt on SimplyML: NEAT evolves controllers that balance a double pendulum on a
// cart, with gravity growing (and friction shrinking) each time the population masters the task.
// Training runs on its own thread; the window never waits for it. Press D for the demo (the best
// agent replaying a reference disturbance sequence), H to hide the controls, Esc to quit.
//
// Options: --size WxH (default 1600x900), --fullscreen, --threads N, --out DIR (genome files,
// default ./pendulum_neat_out), --load GENOME [--conf CONF] [--demo].
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

#include <simplyml/core/app.hpp>
#include <simplyml/ui/ui.hpp>

#include "dashboard.hpp"
#include "training/trainer.hpp"

#ifndef PENDULUM_RES_DIR
#define PENDULUM_RES_DIR "res"
#endif

int main(int argc, char** argv)
{
    sml::AppConfig config;
    config.title          = "Pendulum - Training";
    config.size           = {1600, 900};
    config.antialiasing   = 8;
    config.escToQuit      = true;
    config.cameraControls = false;
    std::string out = "pendulum_neat_out", genome, conf;
    bool        startDemo = false;
    uint32_t    threads   = std::max(1u, std::thread::hardware_concurrency());
    for (int i = 1; i < argc; ++i) {
        auto const arg  = std::string(argv[i]);
        auto const next = [&] { return i + 1 < argc ? std::string(argv[++i]) : std::string{}; };
        if (arg == "--size") {
            auto const s = next();
            config.size  = {static_cast<unsigned>(std::atoi(s.c_str())),
                            static_cast<unsigned>(std::atoi(s.c_str() + s.find('x') + 1))};
        } else if (arg == "--fullscreen") {
            config.fullscreen = true;
        } else if (arg == "--threads") {
            threads = static_cast<uint32_t>(std::max(1, std::atoi(next().c_str())));
        } else if (arg == "--out") {
            out = next();
        } else if (arg == "--load") {
            genome = next();
        } else if (arg == "--conf") {
            conf = next();
        } else if (arg == "--demo") {
            startDemo = true;
        } else {
            std::cerr << "unknown option " << arg << "\n";
            return 1;
        }
    }

    sml::App app{config};
    app.resources().loadTexture("wheel", std::string(PENDULUM_RES_DIR) + "/wheel.png");
    sml::Ui ui{app};
    float const scale = std::min(config.size.x / 2560.0f, config.size.y / 1440.0f);
    std::vector<std::string> labels;
    if (conf::net::control_type == conf::ControlType::Acceleration) {
        labels = {"Position", "Velocity", "Direction_1 x", "Direction_1 y", "Angular_vel_1",
                  "Direction_2 x", "Direction_2 y", "Angular_vel_2", "dot(dir_1, dir_2)"};
    } else {
        labels = {"Position", "Direction_1 x", "Direction_1 y", "Angular_vel_1",
                  "Direction_2 x", "Direction_2 y", "Angular_vel_2", "dot(dir_1, dir_2)"};
    }
    Dashboard dashboard{app, ui, scale, labels,
                        conf::net::control_type == conf::ControlType::Acceleration ? "Output (acceleration)"
                                                                                    : "Output (velocity)"};

    Trainer trainer{app, dashboard, out, threads};
    if (!genome.empty()) {
        trainer.loadGenome(genome);
    }
    if (!conf.empty()) {
        trainer.loadConf(conf);
    }
    if (startDemo) {
        app.controls().set("demo", 1.0);
    }

    app.start([&](sml::App&, float) { dashboard.update(); });
    trainer.run(); // until the window closes
    app.join();
}
