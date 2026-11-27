#include "Test.hpp"
#include "core/Document.hpp"
#include "engine2d/AnimationPlayer.hpp"

using namespace xeldi;

static Timeline makeTl(int n, int ms) {
    Timeline t;
    t.frames.assign(n, Frame{ms});
    return t;
}

TEST("bucle forward respeta duraciones") {
    Timeline tl = makeTl(4, 100);
    tl.frames[1].durationMs = 300;
    AnimationPlayer p;
    p.setRange(0, 3);
    CHECK_EQ(p.frame(), 0);
    p.advance(tl, 99);
    CHECK_EQ(p.frame(), 0);
    p.advance(tl, 1);
    CHECK_EQ(p.frame(), 1);
    p.advance(tl, 299);
    CHECK_EQ(p.frame(), 1);
    p.advance(tl, 1);
    CHECK_EQ(p.frame(), 2);
    p.advance(tl, 200);
    CHECK_EQ(p.frame(), 0);  // vuelve al inicio
}

TEST("ping-pong y reverse") {
    Timeline tl = makeTl(3, 10);
    AnimationPlayer p;
    p.setRange(0, 2, LoopMode::PingPong);
    std::vector<int> seq;
    for (int i = 0; i < 6; ++i) { seq.push_back(p.frame()); p.step(); }
    CHECK((seq == std::vector<int>{0, 1, 2, 1, 0, 1}));
    p.setRange(0, 2, LoopMode::Reverse);
    CHECK_EQ(p.frame(), 2);
    p.step();
    CHECK_EQ(p.frame(), 1);
}

TEST("fps_override del tag") {
    Timeline tl = makeTl(4, 1000);
    AnimationTag t{"idle", 1, 2, LoopMode::Forward, 10};
    AnimationPlayer p;
    p.setTag(t);
    CHECK_EQ(p.frame(), 1);
    p.advance(tl, 100);
    CHECK_EQ(p.frame(), 2);
}

TEST("tags se ajustan al insertar y borrar frames") {
    Document d = Document::createBlank(4, 4);
    for (int i = 0; i < 5; ++i) d.insertFrame(d.frameCount());
    d.timeline.tags.push_back({"idle", 2, 4});
    d.insertFrame(0);
    CHECK_EQ(d.timeline.tags[0].from, 3);
    CHECK_EQ(d.timeline.tags[0].to, 5);
    d.removeFrame(4);
    CHECK_EQ(d.timeline.tags[0].to, 4);
    CHECK(d.validate());
}

TEST_MAIN()
