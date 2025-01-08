#include <gtest/gtest.h>
#include "visuals.h"
#include "customentity.h"

const char visuals[] = R"(
{
	"Vortigaunt.ZapBeam": {
		"color": [242, 0, 213],
		"alpha": 200,
		"sprite": "sprites/xsmoke3.spr",
		"noise": 40,
		"width": 60,
		"beamflags": ["sine", "solid"]
	},
	"Bullsquid.Spit": {
		"sprite": "sprites/e-tele1.spr",
		"scale": 0.25,
		"rendermode": "Glow",
		"renderfx": "Constant Glow",
		"framerate": 15.0
	},
	"Hornet.Model": {
		"renderfx": "hologram",
	},
	"Controller.EnergyBallLight": {
		"radius": [32, 48]
	},
}
)";

TEST(Visuals, Parse) {
	VisualSystem s;
	ASSERT_TRUE(s.ReadFromContents(visuals, ""));

	{
		const Visual* zapBeam = s.GetVisual("Vortigaunt.ZapBeam");
		ASSERT_TRUE(zapBeam != nullptr);
		EXPECT_TRUE(zapBeam->HasDefined(Visual::COLOR_DEFINED));
		EXPECT_EQ(zapBeam->rendercolor, Color3(242, 0, 213));
		EXPECT_TRUE(zapBeam->HasDefined(Visual::ALPHA_DEFINED));
		EXPECT_EQ(zapBeam->renderamt, 200);
		EXPECT_TRUE(zapBeam->HasDefined(Visual::MODEL_DEFINED));
		EXPECT_STREQ(zapBeam->model, "sprites/xsmoke3.spr");
		EXPECT_TRUE(zapBeam->HasDefined(Visual::BEAMNOISE_DEFINED));
		EXPECT_EQ(zapBeam->beamNoise, 40);
		EXPECT_TRUE(zapBeam->HasDefined(Visual::BEAMWIDTH_DEFINED));
		EXPECT_EQ(zapBeam->beamWidth, 60);
		EXPECT_TRUE(zapBeam->HasDefined(Visual::BEAMFLAGS_DEFINED));
		EXPECT_EQ(zapBeam->beamFlags, BEAM_FSINE|BEAM_FSOLID);

		EXPECT_FALSE(zapBeam->HasDefined(Visual::RENDERMODE_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::RENDERFX_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::SCALE_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::FRAMERATE_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::BEAMSCROLLRATE_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::LIFE_DEFINED));
		EXPECT_FALSE(zapBeam->HasDefined(Visual::RADIUS_DEFINED));
	}

	{
		const Visual* bullsquidSpit = s.GetVisual("Bullsquid.Spit");
		ASSERT_TRUE(bullsquidSpit != nullptr);
		EXPECT_STREQ(bullsquidSpit->model, "sprites/e-tele1.spr");
		EXPECT_TRUE(bullsquidSpit->HasDefined(Visual::SCALE_DEFINED));
		EXPECT_EQ(bullsquidSpit->scale, 0.25f);
		EXPECT_TRUE(bullsquidSpit->HasDefined(Visual::RENDERMODE_DEFINED));
		EXPECT_EQ(bullsquidSpit->rendermode, kRenderGlow);
		EXPECT_TRUE(bullsquidSpit->HasDefined(Visual::RENDERFX_DEFINED));
		EXPECT_EQ(bullsquidSpit->renderfx, kRenderFxNoDissipation);
		EXPECT_TRUE(bullsquidSpit->HasDefined(Visual::FRAMERATE_DEFINED));
		EXPECT_EQ(bullsquidSpit->framerate, 15.0f);
	}

	{
		const Visual* hornetModel = s.GetVisual("Hornet.Model");
		ASSERT_TRUE(hornetModel != nullptr);
		EXPECT_EQ(hornetModel->renderfx,kRenderFxHologram);
	}

	{
		const Visual* energyBallLight = s.GetVisual("Controller.EnergyBallLight");
		ASSERT_TRUE(energyBallLight != nullptr);
		EXPECT_TRUE(energyBallLight->HasDefined(Visual::RADIUS_DEFINED));
		EXPECT_EQ(energyBallLight->radius, IntRange(32, 48));
	}

	{
		const Visual* nonexistent = s.GetVisual("Nonexistent");
		ASSERT_TRUE(nonexistent == nullptr);
	}
}
