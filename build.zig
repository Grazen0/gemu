const std = @import("std");

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const argparse = b.addLibrary(.{
        .name = "argparse",
        .root_module = b.createModule(.{
            .link_libc = true,
            .target = target,
            .optimize = optimize,
        }),
    });
    argparse.root_module.addCSourceFile(.{
        .file = b.path("external/argparse/argparse.c"),
    });
    argparse.root_module.addIncludePath(b.path("external/argparse"));

    const exe = b.addExecutable(.{
        .name = "gemu",
        .root_module = b.createModule(.{
            .link_libc = true,
            .target = target,
            .optimize = optimize,
        }),
    });

    exe.root_module.addCSourceFiles(.{
        .files = &.{
            "src/main.c",
            "src/cpu.c",
            "src/data.c",
            "src/frontend.c",
            "src/game_boy.c",
            "src/instructions.c",
            "src/log.c",
            "src/macros.c",
            "src/num.c",
            "src/sdl.c",
        },
        .flags = &.{
            "-std=c23",
            "-g",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
        },
    });

    exe.root_module.addIncludePath(b.path("src"));
    exe.root_module.addIncludePath(b.path("external/argparse"));
    exe.root_module.linkLibrary(argparse);

    exe.root_module.linkSystemLibrary("SDL3", .{
        .preferred_link_mode = .static,
        .use_pkg_config = .force,
    });
    b.installArtifact(exe);

    const run_cmd = b.addRunArtifact(exe);
    run_cmd.step.dependOn(b.getInstallStep());

    if (b.args) |args|
        run_cmd.addArgs(args);

    const run_step = b.step("run", "Run the app");
    run_step.dependOn(&run_cmd.step);
}
