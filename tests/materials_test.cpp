#include <gtest/gtest.h>
#include "error_collector.h"
#include "tex_materials.h"

const char materials[] = R"(
{
	"materials": {
		"R": {
			"step": {
				"right": ["player/pl_rock0.wav", "player/pl_rock2.wav"],
				"left": ["player/pl_rock1.wav", "player/pl_rock3.wav"],
				"walking": {
					"time": 700,
					"volume": 0.5
				},
				"running": {
					"time": 350,
					"volume": 0.7
				}
			},
			"hit": {
				"waves": ["debris/rock1.wav", "debris/rock2.wav", "debris/rock3.wav"],
				"volume": 0.7,
				"volume_bar": 0.4,
				"attenuation": 1.25,
				"wallpuff_color": [170, 85, 0],
				"play_sparks": true
			}
		},
		"X": {
			"hit": {
				"wallpuff_color": [85, 255, 0]
			}
		},
		"S": {
			"hit": {
				"allow_wallpuff": false,
				"allow_weapon_sparks": false
			}
		}
	},
	"ladder_step": {
		"right": ["player/custom_ladder1.wav"],
		"left": ["player/custom_ladder2.wav"],
	},
	"wade_step": {
		"right": ["player/wade_ladder1.wav"],
		"left": ["player/wade_ladder2.wav"],
	},
	"default_material": "T",
	"default_step_material": "C",
	"slosh_material": "O",
	"flesh_material": "D"
}
)";

TEST(Materials, Default) {
	MaterialRegistry r;
	r.FillDefaults();

	EXPECT_EQ(r.DefaultMaterial(), 'C');
	EXPECT_EQ(r.FleshMaterial(), 'F');

	const MaterialData* data = r.GetMaterialData('C');
	EXPECT_TRUE(data != nullptr);

	EXPECT_TRUE(r.GetMaterialData('_') == nullptr);
	EXPECT_EQ(r.GetMaterialDataWithFallback('_'), data);

	const MaterialStepData* stepData = r.GetMaterialStepData('W');
	EXPECT_TRUE(stepData != nullptr);

	EXPECT_EQ(stepData, &data->step);
}

TEST(Materials, Parse) {
	MaterialRegistry r;
	r.FillDefaults();
	ASSERT_TRUE(r.ReadFromContents(materials, ""));

	EXPECT_EQ(r.DefaultMaterial(), 'T');
	EXPECT_EQ(r.FleshMaterial(), 'D');

	{
		const MaterialData* data = r.GetMaterialDataWithFallback('_');
		ASSERT_TRUE(data != nullptr);
		EXPECT_EQ(data, r.GetMaterialData('T'));
	}

	{
		const MaterialData* data = r.GetMaterialData('R');
		ASSERT_TRUE(data != nullptr);

		EXPECT_EQ(data->step.right.size(), 2);
		EXPECT_EQ(data->step.right[0], "player/pl_rock0.wav");
		EXPECT_EQ(data->step.right[1], "player/pl_rock2.wav");

		EXPECT_EQ(data->step.left.size(), 2);
		EXPECT_EQ(data->step.left[0], "player/pl_rock1.wav");
		EXPECT_EQ(data->step.left[1], "player/pl_rock3.wav");

		EXPECT_EQ(data->step.walking.timeMsec, 700);
		EXPECT_EQ(data->step.walking.volume, 0.5f);

		EXPECT_EQ(data->step.running.timeMsec, 350);
		EXPECT_EQ(data->step.running.volume, 0.7f);

		EXPECT_EQ(data->hit.waves.size(), 3);
		EXPECT_EQ(data->hit.waves[0], "debris/rock1.wav");
		EXPECT_EQ(data->hit.waves[1], "debris/rock2.wav");
		EXPECT_EQ(data->hit.waves[2], "debris/rock3.wav");

		EXPECT_EQ(data->hit.volume, 0.7f);
		EXPECT_EQ(data->hit.volumebar, 0.4f);
		EXPECT_EQ(data->hit.attn, 1.25f);
		EXPECT_EQ(data->hit.wallpuffColor, Color3(170, 85, 0));
		EXPECT_TRUE(data->hit.playSparks);
	}

	{
		const MaterialData* data = r.GetMaterialData('X');
		ASSERT_TRUE(data != nullptr);

		const MaterialStepData* stepData = r.GetMaterialStepData('X');
		ASSERT_TRUE(stepData != nullptr);

		const MaterialData* defaultData = r.GetMaterialData('C');
		ASSERT_TRUE(defaultData != nullptr);

		EXPECT_EQ(stepData, &defaultData->step);
	}

	{
		const MaterialData* data = r.GetMaterialData('S');
		ASSERT_TRUE(data != nullptr);

		EXPECT_FALSE(data->hit.allowWallpuff);
		EXPECT_FALSE(data->hit.allowWeaponSparks);
	}

	{
		const MaterialData* data = r.GetMaterialData('O');
		ASSERT_TRUE(data != nullptr);

		const MaterialStepData* stepData = r.GetSloshStepData();
		ASSERT_TRUE(stepData != nullptr);

		EXPECT_EQ(&data->step, stepData);
	}

	{
		const MaterialStepData* data = r.GetLadderStepData();
		ASSERT_TRUE(data != nullptr);

		EXPECT_EQ(data->right.size(), 1);
		EXPECT_EQ(data->right[0], "player/custom_ladder1.wav");

		EXPECT_EQ(data->left.size(), 1);
		EXPECT_EQ(data->left[0], "player/custom_ladder2.wav");
	}

	{
		const MaterialStepData* data = r.GetWadeStepData();
		ASSERT_TRUE(data != nullptr);

		EXPECT_EQ(data->right.size(), 1);
		EXPECT_EQ(data->right[0], "player/wade_ladder1.wav");

		EXPECT_EQ(data->left.size(), 1);
		EXPECT_EQ(data->left[0], "player/wade_ladder2.wav");
	}
}

const char invalidMaterials[] = R"(
{
	"materials": {
		"ManyLetters": {
		}
	}
}
)";

TEST(Materials, WrongMaterialName) {
	g_errorCollector.Clear();

	MaterialRegistry r;
	r.ReadFromContents(invalidMaterials, "");
	EXPECT_TRUE(g_errorCollector.HasErrors());
}
