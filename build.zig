const std = @import("std");

const src = [_][]const u8{ "libco/libco.c", "libmicrokitco.c" };

const src_aarch64 = [_][]const u8{
    "libco/aarch64.c",
};

const src_riscv64 = [_][]const u8{
    "libco/riscv64.c",
};

const src_x86_64 = [_][]const u8{
    "libco/amd64.c",
};

pub fn build(b: *std.Build) !void {
    const target = b.standardTargetOptions(.{});
    const optimize = b.standardOptimizeOption(.{});

    const microkit_include = b.option([]const u8, "libmicrokit_include", "Include path to libmicrokit") orelse "";
    const libmicrokitco_opts_include = b.option([]const u8, "libmicrokitco_opts", "Path to libmicrokitco_opts.h") orelse "";

    const libmicrokitco = b.addStaticLibrary(.{
        .name = "microkitco",
        .target = target,
        .optimize = optimize,
    });

    // Find architecture specific source files
    const arch_src = switch (target.result.cpu.arch) {
        .aarch64 => src_aarch64,
        .riscv64 => src_riscv64,
        .x86_64 => src_x86_64,
        else => {
            std.debug.print("Unexpected target architecture for libco: {}\n", .{target.result.cpu.arch});
            return error.UnexpectedCpuArch;
        },
    };

    libmicrokitco.addCSourceFiles(.{ .files = &src, .flags = &.{} });
    libmicrokitco.addCSourceFiles(.{ .files = &arch_src, .flags = &.{} });
    libmicrokitco.addIncludePath(b.path("libco"));
    libmicrokitco.addIncludePath(b.path("libhostedqueue"));
    libmicrokitco.addIncludePath(.{ .cwd_relative = microkit_include });
    libmicrokitco.addIncludePath(.{ .cwd_relative = libmicrokitco_opts_include });

    libmicrokitco.installHeader(b.path("libmicrokitco.h"), "libmicrokitco.h");
    libmicrokitco.installHeadersDirectory(b.path("libhostedqueue"), "libhostedqueue", .{});

    b.installArtifact(libmicrokitco);
}
