#!/usr/bin/env bash
cd "$(dirname "$0")"
flatpak-builder --repo=build/repo --force-clean build/out com.enzo3d.Enzo.yml
flatpak build-bundle build/repo build/enzo.flatpak com.enzo3d.Enzo
