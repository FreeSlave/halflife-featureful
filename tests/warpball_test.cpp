#include <gtest/gtest.h>
#include "warpball.h"

const char warpballs[] = R"(
{
	"entity_mappings": {
		"auto": {
			// the mapping between monster name and template name
			"monster_alien_slave": "xen",
			"monster_alien_grunt": "xen_big",
			"monster_headcrab": "xen_small",
			"monster_pitdrone": "racex",
			"monster_shocktrooper": "racex_big",
			"default": "xen"
		},
		"alt_mapping": {
			"default": "xen_alt"
		}
	},
	"templates": {
		"xen": {
			"sprite1": {
				"sprite": "sprites/fexplo_1.spr",
				"alpha": 225,
				"color": "77 210 130",
				"framerate": 12,
			},
			"sprite2": {
				"sprite": "sprites/xflare_1.spr",
				"alpha": 200,
				"color": "70 220 120",
				"framerate": 10,
				"scale": 1.1
			},
			"sound1": {
				"sound": "debris/beamstart_2.wav",
			},
			"sound2": {
				"sound": "debris/beamstart_7.wav",
			},
			"beam": {
				"sprite": "sprites/ligtning.spr",
				"color": "20 240 20",
				"alpha": 240,
				"width": 30,
				"noise": 65,
				"life": [0.8, 1.6]
			},
			"beam_radius": 200,
			"beam_count": [10, 20],
			"light": {
				"color": "80 210 130",
				"radius": 200
			},
			"spawn_delay": 0.4
		},
		"xen_alt": {
			"inherits": "xen",
			"beam": {
				"color": [217, 226, 146],
				"alpha": 128,
				"width": 25,
				"noise": 50
			},
			"light": {
				"color": "39 209 137"
			},
			"sprite1": {
				"color": "65 209 61"
			},
			"sprite2": {
				"color": "159 240 214"
			},
			"sound1": {
				"sound": "debris/alien_teleport.wav"
			},
			"sound2": null,
			"beam_count": 8
		},
		"xen_small": {
			"inherits": "xen",
			"sprite1": {
				"scale": 0.8
			},
			"sprite2": {
				"scale": 0.9
			},
			"sound1": {
				"attenuation": 1.0,
				"volume": 0.9,
				"pitch": 105
			},
			"sound2": {
				"attenuation": "static",
				"volume": 0.8,
				"pitch": 110
			},
			"light": {
				"radius": 160
			},
			"beam_count": [10, 15],
			"spawn_delay": 0.3
		},
		"xen_big": {
			"inherits": "xen",
			"sprite1": {
				"scale": 1.25
			},
			"sprite2": {
				"scale": 1.25
			},
			"shake": {
				"radius": 200,
				"duration": 1.0,
				"frequency": 160,
				"amplitude": 6
			},
			"sound1": {
				"attenuation": 0.6
			},
			"sound2": {
				"attenuation": 0.6
			},
			"position": {
				"vertical_shift": 42
			}
		},
		"racex": {
			"inherits": "xen",
			"sprite1": {
				"sprite": "sprites/xflare2.spr",
				"color": "200 100 200"
			},
			"sprite2": null,
			"beam": {
				"color": "240 80 160"
			},
			"light": {
				"color": "200 100 200"
			},
			"sound1": {
				"pitch": 105
			},
			"sound2": {
				"pitch": 105
			}
		},
		"racex_big": {
			"inherits": "racex",
			"sprite1": {
				"scale": 1.25
			},
			"beam_radius": 256,
				"shake": {
				"radius": 192,
				"duration": 1.0,
				"frequency": 160,
				"amplitude": 6
			},
			"sound1": {
				"attenuation": "none"
			},
			"sound2": {
				"attenuation": 0.6
			},
			"position": {
				"vertical_shift": 40
			}
		}
	}
}
)";

TEST(WarpballTemplates, Parse) {
	WarpballTemplateCatalog c;
	ASSERT_TRUE(c.ReadFromContents(warpballs, ""));

	const WarpballTemplate* xen = c.FindWarpballTemplate("xen");
	ASSERT_TRUE(xen != nullptr);

	EXPECT_STREQ(xen->sprite1.sprite, "sprites/fexplo_1.spr");
	EXPECT_EQ(xen->sprite1.alpha, 225);
	EXPECT_EQ(xen->sprite1.color, Color3(77, 210, 130));
	EXPECT_EQ(xen->sprite1.framerate, 12.0f);
	EXPECT_EQ(xen->sprite1.scale, 1.0f);
	EXPECT_EQ(xen->sprite1.rendermode, kRenderGlow);
	EXPECT_EQ(xen->sprite1.renderfx, kRenderFxNoDissipation);

	EXPECT_STREQ(xen->sprite2.sprite, "sprites/xflare_1.spr");
	EXPECT_EQ(xen->sprite2.alpha, 200);
	EXPECT_EQ(xen->sprite2.color, Color3(70, 220, 120));
	EXPECT_EQ(xen->sprite2.framerate, 10.f);
	EXPECT_EQ(xen->sprite2.scale, 1.1f);

	EXPECT_STREQ(xen->sound1.sound, "debris/beamstart_2.wav");
	EXPECT_EQ(xen->sound1.pitch, 100);
	EXPECT_EQ(xen->sound1.volume, 1.0f);
	EXPECT_EQ(xen->sound1.attenuation, 0.8f);

	EXPECT_STREQ(xen->sound2.sound, "debris/beamstart_7.wav");

	EXPECT_STREQ(xen->beam.sprite, "sprites/ligtning.spr");
	EXPECT_EQ(xen->beam.color, Color3(20, 240, 20));
	EXPECT_EQ(xen->beam.alpha, 240);
	EXPECT_EQ(xen->beam.width, 30);
	EXPECT_EQ(xen->beam.noise, 65);
	EXPECT_EQ(xen->beam.life, FloatRange(0.8f, 1.6f));

	EXPECT_EQ(xen->beamRadius, 200);
	EXPECT_EQ(xen->beamCount, IntRange(10, 20));
	EXPECT_EQ(xen->spawnDelay, 0.4f);

	EXPECT_EQ(xen->light.color, Color3(80, 210, 130));
	EXPECT_EQ(xen->light.radius, 200);

	EXPECT_FALSE(xen->shake.IsDefined());
	EXPECT_FALSE(xen->aiSound.IsDefined());
	EXPECT_FALSE(xen->position.IsDefined());

	{
		const WarpballTemplate* t = c.FindWarpballTemplate("xen_alt");
		ASSERT_TRUE(t != nullptr);

		EXPECT_EQ(t->beam.color, Color3(217, 226, 146));
		EXPECT_EQ(t->beam.alpha, 128);
		EXPECT_EQ(t->beam.width, 25);
		EXPECT_EQ(t->beam.noise, 50);
		EXPECT_EQ(t->beam.life, xen->beam.life);

		EXPECT_EQ(t->light.radius, xen->light.radius);
		EXPECT_EQ(t->light.color, Color3(39, 209, 137));

		EXPECT_TRUE(strcmp(t->sprite1.sprite, xen->sprite1.sprite) == 0);
		EXPECT_EQ(t->sprite1.color, Color3(65, 209, 61));
		EXPECT_EQ(t->sprite1.alpha, xen->sprite1.alpha);
		EXPECT_EQ(t->sprite1.framerate, xen->sprite1.framerate);
		EXPECT_EQ(t->sprite1.scale, xen->sprite1.scale);

		EXPECT_TRUE(strcmp(t->sprite2.sprite, xen->sprite2.sprite) == 0);
		EXPECT_EQ(t->sprite2.color, Color3(159, 240, 214));

		EXPECT_STREQ(t->sound1.sound, "debris/alien_teleport.wav");

		EXPECT_EQ(t->sound2.sound, nullptr);

		EXPECT_EQ(t->beamRadius, xen->beamRadius);
		EXPECT_EQ(t->beamCount, 8);
		EXPECT_EQ(t->spawnDelay, xen->spawnDelay);
	}

	{
		const WarpballTemplate* t = c.FindWarpballTemplate("xen_small");
		ASSERT_TRUE(t != nullptr);

		EXPECT_STREQ(t->sprite1.sprite, xen->sprite1.sprite);
		EXPECT_EQ(t->sprite1.color, xen->sprite1.color);
		EXPECT_EQ(t->sprite1.alpha, xen->sprite1.alpha);
		EXPECT_EQ(t->sprite1.framerate, xen->sprite1.framerate);
		EXPECT_EQ(t->sprite1.scale, 0.8f);

		EXPECT_STREQ(t->sprite2.sprite, xen->sprite2.sprite);
		EXPECT_EQ(t->sprite2.color, xen->sprite2.color);
		EXPECT_EQ(t->sprite2.scale, 0.9f);

		EXPECT_STREQ(t->sound1.sound, xen->sound1.sound);
		EXPECT_EQ(t->sound1.attenuation, 1.0f);
		EXPECT_EQ(t->sound1.volume, 0.9f);
		EXPECT_EQ(t->sound1.pitch, 105);

		EXPECT_STREQ(t->sound2.sound, xen->sound2.sound);
		EXPECT_EQ(t->sound2.attenuation, ATTN_STATIC);
		EXPECT_EQ(t->sound2.volume, 0.8f);
		EXPECT_EQ(t->sound2.pitch, 110);

		EXPECT_EQ(t->light.color, xen->light.color);
		EXPECT_EQ(t->light.radius, 160);

		EXPECT_EQ(t->beamCount, IntRange(10, 15));
		EXPECT_EQ(t->spawnDelay, 0.3f);
	}

	{
		const WarpballTemplate* t = c.FindWarpballTemplate("xen_big");
		ASSERT_TRUE(t != nullptr);

		EXPECT_EQ(t->sprite1.scale, 1.25f);

		EXPECT_TRUE(t->shake.IsDefined());
		EXPECT_EQ(t->shake.radius, 200);
		EXPECT_EQ(t->shake.duration, 1.0f);
		EXPECT_EQ(t->shake.frequency, 160);
		EXPECT_EQ(t->shake.amplitude, 6);

		EXPECT_EQ(t->sound1.attenuation, 0.6f);

		EXPECT_TRUE(t->position.IsDefined());
		EXPECT_EQ(t->position.verticalShift, 42);
	}

	{
		const WarpballTemplate* t = c.FindWarpballTemplate("racex_big");
		ASSERT_TRUE(t != nullptr);
		EXPECT_STREQ(t->sprite1.sprite, "sprites/xflare2.spr");
		EXPECT_EQ(t->sprite1.color, Color3(200, 100, 200));

		EXPECT_EQ(t->sprite2.sprite, nullptr);
		EXPECT_EQ(t->beam.color, Color3(240, 80, 160));
		EXPECT_EQ(t->sound1.pitch, 105);
		EXPECT_EQ(t->sound1.attenuation, ATTN_NONE);

		EXPECT_TRUE(t->shake.IsDefined());
	}

	{
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_unknown"), xen);
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_alien_slave"), xen);
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_alien_grunt"), c.FindWarpballTemplate("xen_big"));
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_headcrab"), c.FindWarpballTemplate("xen_small"));
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_pitdrone"), c.FindWarpballTemplate("racex"));
		EXPECT_EQ(c.FindWarpballTemplate("auto", "monster_shocktrooper"), c.FindWarpballTemplate("racex_big"));

		EXPECT_EQ(c.FindWarpballTemplate("alt_mapping", "monster_unknown"), c.FindWarpballTemplate("xen_alt"));
	}
}

const char looped[] = R"(
{
	"templates": {
		"xen": {
			"inherits": "xen_alt"
		},
		"xen_alt": {
			"inherits": "xen_small",
		},
		"xen_small": {
			"inherits": "xen",
		}
	}
}
)";

TEST(WarpballTemplates, DetectLoop) {
	WarpballTemplateCatalog c;
	ASSERT_FALSE(c.ReadFromContents(looped, ""));
}

const char missingParent[] = R"(
{
	"templates": {
		"xen": {},
		"xen_alt": {
			"inherits": "nonexistent",
		}
	}
}
)";

TEST(WarpballTemplates, MissingParent) {
	WarpballTemplateCatalog c;
	ASSERT_FALSE(c.ReadFromContents(missingParent, ""));
}
