#include "test_framework.hpp"
#include "peregrine/template.hpp"

using namespace peregrine;

TEST_CASE(TemplateSuite, ScalarVariableSubstitution) {
    TemplateContext ctx;
    ctx.set("framework", "Peregrine");
    ctx.set("language", "C++11");
    ctx.set("version", "1.0");

    std::string tmpl = "Welcome to {{framework}} running on {{language}} (v{{version}})!";
    std::string rendered = render_string(tmpl, ctx);

    EXPECT_EQ(rendered, "Welcome to Peregrine running on C++11 (v1.0)!");
}

TEST_CASE(TemplateSuite, MissingVariableHandling) {
    TemplateContext ctx;
    ctx.set("existing", "Found");

    std::string tmpl = "Check: {{existing}}, but missing: {{missing}}.";
    std::string rendered = render_string(tmpl, ctx);

    EXPECT_EQ(rendered, "Check: Found, but missing: .");
}

TEST_CASE(TemplateSuite, EachCollectionLoop) {
    TemplateContext ctx;
    ctx.set("group_name", "Developers");

    ctx.append("members", {{"id", "1"}, {"name", "Alice"}});
    ctx.append("members", {{"id", "2"}, {"name", "Bob"}});
    ctx.append("members", {{"id", "3"}, {"name", "Charlie"}});

    std::string tmpl = "Group: {{group_name}}\n"
                       "Members:\n"
                       "{{#each members}}"
                       " - [{{this.id}}] {{this.name}}\n"
                       "{{/each}}"
                       "End of list.";

    std::string rendered = render_string(tmpl, ctx);

    EXPECT_CONTAINS(rendered, "Group: Developers");
    EXPECT_CONTAINS(rendered, " - [1] Alice\n");
    EXPECT_CONTAINS(rendered, " - [2] Bob\n");
    EXPECT_CONTAINS(rendered, " - [3] Charlie\n");
    EXPECT_CONTAINS(rendered, "End of list.");
}

TEST_CASE(TemplateSuite, EmptyEachLoop) {
    TemplateContext ctx;
    ctx.set("title", "Empty Group");

    std::string tmpl = "<h1>{{title}}</h1><ul>{{#each users}}<li>{{this.name}}</li>{{/each}}</ul>";
    std::string rendered = render_string(tmpl, ctx);

    EXPECT_EQ(rendered, "<h1>Empty Group</h1><ul></ul>");
}

TEST_CASE(TemplateSuite, PlainTextWithNoTags) {
    TemplateContext ctx;
    std::string tmpl = "Plain text with no special brackets.";
    EXPECT_EQ(render_string(tmpl, ctx), tmpl);
}
