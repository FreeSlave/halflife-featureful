#include <gtest/gtest.h>
#include "ent_templates.h"
#include "skilldata.h"
#include "skill.h"

#include <algorithm>
#include <vector>

const char skillFileContents[] = R"(
// Comment

sk_hgrunt_health1  "50"
sk_hgrunt_health2  "60"
sk_hgrunt_health3  "80"

// More comment
sk_islave_dmg_zap1 10
sk_islave_dmg_zap2 12
sk_islave_dmg_zap3 15

sk_barney_health "35"

sk_plr_crowbar 10

islave_health1	"30"
islave_health2	"40"
islave_health3	"60"

scientist_health "20"

plr_9mm_bullet 8

hgrunt_gren_launch_delay1 "6"
hgrunt_gren_launch_delay2 "5"
hgrunt_gren_launch_delay3 "2, 5"
)";

const char entities[] = R"(
{
	"vort": {
		"skill": {
			"islave_health": 45,
			"islave_zap_rate": [1.2, 1.3, 1.4],
			"scientist_health": "*2",
			"hgrunt_health": "barney_health"
		}
	}
}
)";

TEST(SkillData, Parse)
{
	SoundScriptSystem ss;
	VisualSystem vs;
	EntTemplateSystem es;
	es.SetSoundScriptSystem(&ss);
	es.SetVisualSystem(&vs);

	ASSERT_TRUE(es.ReadFromContents(entities, ""));

	const char* entTemplateName = "vort";
	const EntTemplate* entTemplate = es.GetTemplate(entTemplateName);

	ASSERT_TRUE(entTemplate != nullptr);

	g_SkillData.Clear();

	g_iSkillLevel = SKILL_EASY;

	size_t fileSize = strlen(skillFileContents);
	std::vector<unsigned char> skillContents(fileSize);
	std::copy_n(skillFileContents, fileSize, skillContents.begin());

	ParseSkillCfg(skillContents.data(), (int)fileSize, "skill.cfg");

	g_SkillData.ProvideFallback("barney_health", 40);
	g_SkillData.ProvideFallback("otis_health", "barney_health");
	g_SkillData.ProvideFallback("massassin_health", "hgrunt_health");
	g_SkillData.ProvideFallback("islave_zap_rate", 1.0f, 1.25f, 1.5f);
	g_SkillData.ProvideFallbackWithFactor("islave_dmg_coil", "islave_dmg_zap", 2.5f);

	EXPECT_EQ(::GetSkillValue("hgrunt_health"), 50.0f);
	EXPECT_EQ(::GetSkillValue("massassin_health"), 50.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_zap"), 10.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_coil"), 25.0f);
	EXPECT_EQ(::GetSkillValue("islave_zap_rate"), 1.0f);
	EXPECT_EQ(::GetSkillValue("barney_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("otis_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("plr_crowbar"), 10.0f);
	EXPECT_EQ(::GetSkillValue("islave_health"), 30.0f);
	EXPECT_EQ(::GetSkillValue("scientist_health"), 20.0f);
	EXPECT_EQ(::GetSkillValue("plr_9mm_bullet"), 8.0f);
	EXPECT_EQ(::GetSkillValue("hgrunt_gren_launch_delay"), 6.0f);

	EXPECT_EQ(::GetSkillValue("islave_health", entTemplate, entTemplateName), 45.0f);
	EXPECT_EQ(::GetSkillValue("islave_zap_rate", entTemplate, entTemplateName), 1.2f);
	EXPECT_EQ(::GetSkillValue("scientist_health", entTemplate, entTemplateName), 40.0f);
	EXPECT_EQ(::GetSkillValue("hgrunt_health", entTemplate, entTemplateName), 35.0f);

	g_iSkillLevel = SKILL_MEDIUM;

	EXPECT_EQ(::GetSkillValue("hgrunt_health"), 60.0f);
	EXPECT_EQ(::GetSkillValue("massassin_health"), 60.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_zap"), 12.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_coil"), 30.0f);
	EXPECT_EQ(::GetSkillValue("islave_zap_rate"), 1.25f);
	EXPECT_EQ(::GetSkillValue("barney_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("otis_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("plr_crowbar"), 10.0f);
	EXPECT_EQ(::GetSkillValue("islave_health"), 40.0f);
	EXPECT_EQ(::GetSkillValue("scientist_health"), 20.0f);
	EXPECT_EQ(::GetSkillValue("plr_9mm_bullet"), 8.0f);
	EXPECT_EQ(::GetSkillValue("hgrunt_gren_launch_delay"), 5.0f);

	g_iSkillLevel = SKILL_HARD;

	EXPECT_EQ(::GetSkillValue("hgrunt_health"), 80.0f);
	EXPECT_EQ(::GetSkillValue("massassin_health"), 80.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_zap"), 15.0f);
	EXPECT_EQ(::GetSkillValue("islave_dmg_coil"), 37.5f);
	EXPECT_EQ(::GetSkillValue("islave_zap_rate"), 1.5f);
	EXPECT_EQ(::GetSkillValue("barney_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("otis_health"), 35.0f);
	EXPECT_EQ(::GetSkillValue("plr_crowbar"), 10.0f);
	EXPECT_EQ(::GetSkillValue("islave_health"), 60.0f);
	EXPECT_EQ(::GetSkillValue("scientist_health"), 20.0f);
	EXPECT_EQ(::GetSkillValue("plr_9mm_bullet"), 8.0f);
	EXPECT_EQ(::GetSkillValueRange("hgrunt_gren_launch_delay"), FloatRange(2.0f, 5.0f));

	EXPECT_EQ(::GetSkillValueRange("hgrunt_gren_launch_delay"), ::GetSkillValueRange("sk_hgrunt_gren_launch_delay"));
}
