using UnityEngine;
using UnityEditor;
using System.IO;
using System;

namespace GModParticleExporter
{
    public class GModParticleExporter : EditorWindow
    {
        private ParticleSystem selectedParticleSystem;
        private string exportPath = "Assets/Export/";
        private Vector2 scrollPosition;

        [MenuItem("Tools/GMod Particle Exporter")]
        public static void ShowWindow()
        {
            GetWindow<GModParticleExporter>("GMod Particle Exporter");
        }

        private void OnGUI()
        {
            scrollPosition = EditorGUILayout.BeginScrollView(scrollPosition);

            GUILayout.Label("GMod Particle System Exporter", EditorStyles.boldLabel);
            EditorGUILayout.Space();

            // Particle System Selection
            selectedParticleSystem = (ParticleSystem)EditorGUILayout.ObjectField(
                "Particle System",
                selectedParticleSystem,
                typeof(ParticleSystem),
                true
            );

            EditorGUILayout.Space();

            // Export Path
            EditorGUILayout.BeginHorizontal();
            exportPath = EditorGUILayout.TextField("Export Path", exportPath);
            if (GUILayout.Button("Browse", GUILayout.Width(60)))
            {
                string path = EditorUtility.SaveFolderPanel("Select Export Folder", exportPath, "");
                if (!string.IsNullOrEmpty(path))
                {
                    exportPath = path;
                }
            }
            EditorGUILayout.EndHorizontal();

            EditorGUILayout.Space();
            EditorGUILayout.Space();

            // Export Button
            GUI.enabled = selectedParticleSystem != null;
            if (GUILayout.Button("Export to .gpart", GUILayout.Height(30)))
            {
                ExportParticleSystem();
            }
            GUI.enabled = true;

            EditorGUILayout.Space();

            // Info Display
            if (selectedParticleSystem != null)
            {
                EditorGUILayout.HelpBox("Selected: " + selectedParticleSystem.name, MessageType.Info);

                var main = selectedParticleSystem.main;
                EditorGUILayout.LabelField("Duration:", main.duration.ToString());
                EditorGUILayout.LabelField("Max Particles:", main.maxParticles.ToString());
                EditorGUILayout.LabelField("Looping:", main.loop.ToString());
            }
            else
            {
                EditorGUILayout.HelpBox("Select a Particle System to export", MessageType.Warning);
            }

            EditorGUILayout.EndScrollView();
        }

        private void ExportParticleSystem()
        {
            if (selectedParticleSystem == null)
            {
                EditorUtility.DisplayDialog("Error", "Please select a Particle System", "OK");
                return;
            }

            try
            {
                // Create export data
                var exportData = new ParticleSystemExportData();

                // Metadata
                exportData.metadata = new MetadataData
                {
                    name = selectedParticleSystem.name,
                    version = "1.0",
                    exportDate = DateTime.Now.ToString("yyyy-MM-dd HH:mm:ss"),
                    exporter = "Unity GMod Exporter v1.0"
                };

                // Extract all modules
                ExtractMainModule(exportData, selectedParticleSystem);
                ExtractEmissionModule(exportData, selectedParticleSystem);
                ExtractShapeModule(exportData, selectedParticleSystem);
                ExtractVelocityOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractLimitVelocityOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractForceOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractColorOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractSizeOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractRotationOverLifetimeModule(exportData, selectedParticleSystem);
                ExtractNoiseModule(exportData, selectedParticleSystem);
                ExtractCollisionModule(exportData, selectedParticleSystem);
                ExtractTextureSheetAnimationModule(exportData, selectedParticleSystem);
                ExtractRendererModule(exportData, selectedParticleSystem);
                ExtractSubEmitters(exportData, selectedParticleSystem);

                // Serialize to JSON
                string json = JsonUtility.ToJson(exportData, true);

                // Ensure export directory exists
                if (!Directory.Exists(exportPath))
                {
                    Directory.CreateDirectory(exportPath);
                }

                // Write file
                string filename = selectedParticleSystem.name + ".gpart";
                string fullPath = Path.Combine(exportPath, filename);
                File.WriteAllText(fullPath, json);

                Debug.Log("Particle system exported successfully to: " + fullPath);
                EditorUtility.DisplayDialog("Success", "Particle system exported to:\n" + fullPath, "OK");

                // Export texture if renderer has one
                ExportTexture(selectedParticleSystem);
            }
            catch (Exception e)
            {
                Debug.LogError("Export failed: " + e.Message);
                EditorUtility.DisplayDialog("Export Failed", "Error: " + e.Message, "OK");
            }
        }

        // ========================================================================
        // Module Extraction Methods
        // ========================================================================

        private void ExtractMainModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var main = ps.main;

            data.system = new MainModuleData
            {
                duration = main.duration,
                looping = main.loop,
                prewarm = main.prewarm,
                startDelay = new MinMaxCurveData(main.startDelay),
                startLifetime = new MinMaxCurveData(main.startLifetime),
                startSpeed = new MinMaxCurveData(main.startSpeed),
                startSize = new MinMaxCurveData(main.startSize),
                startSize3D = main.startSize3D,
                startSizeX = new MinMaxCurveData(main.startSizeX),
                startSizeY = new MinMaxCurveData(main.startSizeY),
                startSizeZ = new MinMaxCurveData(main.startSizeZ),
                startRotation = new MinMaxCurveData(main.startRotation),
                startRotation3D = main.startRotation3D,
                startRotationX = new MinMaxCurveData(main.startRotationX),
                startRotationY = new MinMaxCurveData(main.startRotationY),
                startRotationZ = new MinMaxCurveData(main.startRotationZ),
                startColor = new ColorData(main.startColor.color),
                gravityModifier = new MinMaxCurveData(main.gravityModifier),
                simulationSpace = main.simulationSpace.ToString(),
                simulationSpeed = main.simulationSpeed,
                playOnAwake = main.playOnAwake,
                maxParticles = main.maxParticles
            };
        }

        private void ExtractEmissionModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var emission = ps.emission;

            data.emission = new EmissionModuleData
            {
                enabled = emission.enabled,
                rateOverTime = new MinMaxCurveData(emission.rateOverTime),
                rateOverDistance = new MinMaxCurveData(emission.rateOverDistance)
            };

            // Extract bursts
            ParticleSystem.Burst[] bursts = new ParticleSystem.Burst[emission.burstCount];
            emission.GetBursts(bursts);

            foreach (var burst in bursts)
            {
                data.emission.bursts.Add(new BurstData(burst));
            }
        }

        private void ExtractShapeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var shape = ps.shape;

            data.shape = new ShapeModuleData
            {
                enabled = shape.enabled,
                shapeType = shape.shapeType.ToString(),
                angle = shape.angle,
                radius = shape.radius,
                radiusThickness = shape.radiusThickness,
                arc = shape.arc,
                boxScale = new Vector3Data(shape.scale),
                position = new Vector3Data(shape.position),
                rotation = new Vector3Data(shape.rotation),
                scale = new Vector3Data(shape.scale),
                alignToDirection = shape.alignToDirection,
                randomDirectionAmount = shape.randomDirectionAmount,
                sphericalDirectionAmount = shape.sphericalDirectionAmount
            };
        }

        private void ExtractVelocityOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var vel = ps.velocityOverLifetime;

            data.velocityOverLifetime = new VelocityOverLifetimeModuleData
            {
                enabled = vel.enabled,
                x = new MinMaxCurveData(vel.x),
                y = new MinMaxCurveData(vel.y),
                z = new MinMaxCurveData(vel.z),
                space = vel.space.ToString()
            };
        }

        private void ExtractLimitVelocityOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var limit = ps.limitVelocityOverLifetime;

            data.limitVelocityOverLifetime = new LimitVelocityOverLifetimeModuleData
            {
                enabled = limit.enabled,
                limit = new MinMaxCurveData(limit.limit),
                dampen = limit.dampen,
                separateAxes = limit.separateAxes,
                limitX = new MinMaxCurveData(limit.limitX),
                limitY = new MinMaxCurveData(limit.limitY),
                limitZ = new MinMaxCurveData(limit.limitZ)
            };
        }

        private void ExtractForceOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var force = ps.forceOverLifetime;

            data.forceOverLifetime = new ForceOverLifetimeModuleData
            {
                enabled = force.enabled,
                x = new MinMaxCurveData(force.x),
                y = new MinMaxCurveData(force.y),
                z = new MinMaxCurveData(force.z),
                space = force.space.ToString(),
                randomized = force.randomized
            };
        }

        private void ExtractColorOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var color = ps.colorOverLifetime;

            data.colorOverLifetime = new ColorOverLifetimeModuleData
            {
                enabled = color.enabled,
                gradient = new GradientData(color.color.gradient)
            };
        }

        private void ExtractSizeOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var size = ps.sizeOverLifetime;

            data.sizeOverLifetime = new SizeOverLifetimeModuleData
            {
                enabled = size.enabled,
                size = new MinMaxCurveData(size.size),
                separateAxes = size.separateAxes,
                x = new MinMaxCurveData(size.x),
                y = new MinMaxCurveData(size.y),
                z = new MinMaxCurveData(size.z)
            };
        }

        private void ExtractRotationOverLifetimeModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var rotation = ps.rotationOverLifetime;

            data.rotationOverLifetime = new RotationOverLifetimeModuleData
            {
                enabled = rotation.enabled,
                x = new MinMaxCurveData(rotation.x),
                y = new MinMaxCurveData(rotation.y),
                z = new MinMaxCurveData(rotation.z),
                separateAxes = rotation.separateAxes
            };
        }

        private void ExtractNoiseModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var noise = ps.noise;

            data.noise = new NoiseModuleData
            {
                enabled = noise.enabled,
                strength = new MinMaxCurveData(noise.strength),
                frequency = noise.frequency,
                scrollSpeed = noise.scrollSpeed,
                damping = noise.damping,
                octaves = noise.octaveCount,
                octaveMultiplier = noise.octaveMultiplier,
                octaveScale = noise.octaveScale,
                quality = (int)noise.quality,
                separateAxes = noise.separateAxes,
                strengthX = new MinMaxCurveData(noise.strengthX),
                strengthY = new MinMaxCurveData(noise.strengthY),
                strengthZ = new MinMaxCurveData(noise.strengthZ)
            };
        }

        private void ExtractCollisionModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var collision = ps.collision;

            data.collision = new CollisionModuleData
            {
                enabled = collision.enabled,
                type = collision.type.ToString(),
                mode = collision.mode.ToString(),
                dampen = new MinMaxCurveData(collision.dampen),
                bounce = new MinMaxCurveData(collision.bounce),
                lifetimeLoss = new MinMaxCurveData(collision.lifetimeLoss),
                minKillSpeed = collision.minKillSpeed,
                maxKillSpeed = collision.maxKillSpeed,
                radiusScale = collision.radiusScale,
                collidesWithDynamic = collision.collidesWith != 0,
                maxCollisionShapes = collision.maxCollisionShapes
            };
        }

        private void ExtractTextureSheetAnimationModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var texSheet = ps.textureSheetAnimation;

            data.textureSheetAnimation = new TextureSheetAnimationModuleData
            {
                enabled = texSheet.enabled,
                numTilesX = texSheet.numTilesX,
                numTilesY = texSheet.numTilesY,
                animationType = texSheet.animation.ToString(),
                mode = texSheet.mode.ToString(),
                frameOverTime = new MinMaxCurveData(texSheet.frameOverTime),
                startFrame = new MinMaxCurveData(texSheet.startFrame),
                cycleCount = texSheet.cycleCount,
                rowIndex = texSheet.rowIndex
            };
        }

        private void ExtractRendererModule(ParticleSystemExportData data, ParticleSystem ps)
        {
            var renderer = ps.GetComponent<ParticleSystemRenderer>();

            if (renderer != null)
            {
                data.renderer = new RendererModuleData
                {
                    renderMode = renderer.renderMode.ToString(),
                    sortMode = renderer.sortMode.ToString(),
                    minParticleSize = renderer.minParticleSize,
                    maxParticleSize = renderer.maxParticleSize,
                    material = renderer.sharedMaterial != null ? renderer.sharedMaterial.name : "",
                    texture = renderer.sharedMaterial != null && renderer.sharedMaterial.mainTexture != null
                        ? renderer.sharedMaterial.mainTexture.name : "",
                    pivot = new Vector3Data(renderer.pivot),
                    flip = renderer.flip != Vector3.zero,
                    velocityScale = new Vector3Data(renderer.velocityScale),
                    lengthScale = renderer.lengthScale,
                    normalDirection = renderer.normalDirection,
                    sortingOrder = renderer.sortingOrder
                };
            }
        }

        private void ExtractSubEmitters(ParticleSystemExportData data, ParticleSystem ps)
        {
            var subEmitters = ps.subEmitters;

            for (int i = 0; i < subEmitters.subEmittersCount; i++)
            {
                var subEmitter = subEmitters.GetSubEmitterSystem(i);
                var type = subEmitters.GetSubEmitterType(i);

                if (subEmitter != null)
                {
                    data.subEmitters.Add(new SubEmitterData
                    {
                        type = type.ToString(),
                        name = subEmitter.name
                    });
                }
            }
        }

        private void ExportTexture(ParticleSystem ps)
        {
            var renderer = ps.GetComponent<ParticleSystemRenderer>();
            if (renderer == null || renderer.sharedMaterial == null) return;

            var texture = renderer.sharedMaterial.mainTexture as Texture2D;
            if (texture == null) return;

            try
            {
                // Try to export texture as PNG
                string texturePath = Path.Combine(exportPath, texture.name + ".png");

                // Note: This requires the texture to be readable in import settings
                byte[] bytes = texture.EncodeToPNG();
                if (bytes != null)
                {
                    File.WriteAllBytes(texturePath, bytes);
                    Debug.Log("Texture exported to: " + texturePath);
                }
            }
            catch (Exception e)
            {
                Debug.LogWarning("Could not export texture: " + e.Message);
            }
        }
    }
}
