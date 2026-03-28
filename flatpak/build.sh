#!/usr/bin/env bash
cd "$(dirname "$0")"
flatpak-builder --repo=build/repo --force-clean build/out org.enzosoftware.Enzo.yml
flatpak build-bundle build/repo build/enzo.flatpak org.enzosoftware.Enzo
