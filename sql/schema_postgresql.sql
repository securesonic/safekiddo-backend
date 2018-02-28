CREATE TABLE "access_control" (
"id" SERIAL NOT NULL,
"code_child_id" integer NOT NULL,
"day_of_week" integer NOT NULL,
"start_time" time NOT NULL,
"end_time" time NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_access_control_child1_idx" ON "access_control" ("code_child_id");

CREATE TABLE "age_profile" (
"id" SERIAL NOT NULL,
"from_age" integer NOT NULL,
"to_age" integer NOT NULL,
"code_profile_id" integer NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_age_profile_profile1_idx" ON "age_profile" ("code_profile_id");

CREATE TABLE "categories" (
"ext_id" integer NOT NULL,
"name" text NOT NULL,
"code_categories_groups_id" integer NOT NULL,
PRIMARY KEY ("ext_id")
);

CREATE INDEX "fk_categories_categories_groups1_idx" ON "categories" ("code_categories_groups_id");

CREATE TABLE "categories_groups" (
"id" SERIAL NOT NULL,
"name" text NOT NULL,
PRIMARY KEY ("id")
);

CREATE TABLE "child" (
"id" SERIAL NOT NULL,
"name" text NOT NULL,
"birth_date" date NOT NULL,
"code_parent_id" integer NOT NULL,
"uuid" text UNIQUE NOT NULL,
"default_custom_code_profile_id" integer NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_child_parent1_idx" ON "child" ("code_parent_id");
CREATE INDEX "fk_child_profile1_idx" ON "child" ("default_custom_code_profile_id");

CREATE TABLE "device" (
"id" SERIAL NOT NULL,
"code_child_id" integer NOT NULL,
"name" text NOT NULL,
"serial" text NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "device_code_child_id_fk_idx" ON "device" ("code_child_id");

CREATE TABLE "global_blocked_categories_groups_list" (
"id" SERIAL NOT NULL,
"code_categories_groups_id" integer NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_global_blocked_categories_groups_list_copy1_categories_g_idx" ON "global_blocked_categories_groups_list" ("code_categories_groups_id");

CREATE TABLE "global_blocked_url_list" (
"id" SERIAL NOT NULL,
"code_url_id" integer NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_global_blocked_url_list_url1_idx" ON "global_blocked_url_list" ("code_url_id");

CREATE TABLE "parent" (
"id" SERIAL NOT NULL,
"name" text NOT NULL,
PRIMARY KEY ("id")
);

CREATE TABLE "profile" (
"id" SERIAL NOT NULL,
"name" text NOT NULL,
"owner_code_parent_id" integer DEFAULT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_profile_parent1_idx" ON "profile" ("owner_code_parent_id");

CREATE TABLE "profile_categories_groups_list" (
"id" SERIAL NOT NULL,
"code_profile_id" integer NOT NULL,
"code_categories_groups_id" integer NOT NULL,
"blocked" boolean NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_profile_categories_groups_list_profile1_idx" ON "profile_categories_groups_list" ("code_profile_id");
CREATE INDEX "fk_profile_categories_groups_list_categories_groups1_idx" ON "profile_categories_groups_list" ("code_categories_groups_id");

CREATE TABLE "profile_time_list" (
"id" SERIAL NOT NULL,
"code_profile_id" integer NOT NULL,
"code_child_id" integer NOT NULL,
"day_of_week" integer NOT NULL,
"start_time" time NOT NULL,
"end_time" time NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "profile_block_time_list_code_profile_id_idx" ON "profile_time_list" ("code_profile_id");
CREATE INDEX "fk_profile_time_list_child1_idx" ON "profile_time_list" ("code_child_id");

CREATE TABLE "profile_url_list" (
"id" SERIAL NOT NULL,
"code_profile_id" integer NOT NULL,
"code_url_id" integer NOT NULL,
"blocked" boolean NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "profile_url_list_code_url_id_fk_idx" ON "profile_url_list" ("code_url_id");
CREATE INDEX "profile_url_list_code_profile_id_fk_idx" ON "profile_url_list" ("code_profile_id");

CREATE TABLE "tmp_access_categories_group_list" (
"id" SERIAL NOT NULL,
"code_child_id" integer NOT NULL,
"code_categories_groups_id" integer NOT NULL,
"start_date" timestamp NOT NULL,
"duration" time NOT NULL,
"parent_accepted" boolean NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_tmp_access_categories_group_list_child1_idx" ON "tmp_access_categories_group_list" ("code_child_id");
CREATE INDEX "fk_tmp_access_categories_group_list_categories_groups1_idx" ON "tmp_access_categories_group_list" ("code_categories_groups_id");

CREATE TABLE "tmp_access_control" (
"id" SERIAL NOT NULL,
"code_child_id" integer NOT NULL,
"start_date" timestamp NOT NULL,
"duration" time NOT NULL,
"parent_accepted" boolean NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_tmp_access_control_child1_idx" ON "tmp_access_control" ("code_child_id");

CREATE TABLE "tmp_access_url_list" (
"id" SERIAL NOT NULL,
"code_child_id" integer NOT NULL,
"code_url_id" integer NOT NULL,
"start_date" timestamp NOT NULL,
"duration" time NOT NULL,
"parent_accepted" boolean NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_tmp_access_list_child1_idx" ON "tmp_access_url_list" ("code_child_id");
CREATE INDEX "fk_tmp_access_url_list_url1_idx" ON "tmp_access_url_list" ("code_url_id");

CREATE TABLE "url" (
"id" SERIAL NOT NULL,
"address" text NOT NULL,
PRIMARY KEY ("id")
);

CREATE TABLE "webroot_overrides" (
"id" SERIAL NOT NULL,
"code_url_id" integer NOT NULL,
"code_categories_ext_id" integer NOT NULL,
PRIMARY KEY ("id")
);

CREATE INDEX "fk_webroot_overrides_url1_idx" ON "webroot_overrides" ("code_url_id");
CREATE INDEX "fk_webroot_overrides_categories1_idx" ON "webroot_overrides" ("code_categories_ext_id");


ALTER TABLE "access_control" ADD CONSTRAINT "fk_access_control_child1" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "age_profile" ADD CONSTRAINT "fk_age_profile_profile1" FOREIGN KEY ("code_profile_id") REFERENCES "profile" ("id");
ALTER TABLE "categories" ADD CONSTRAINT "fk_categories_categories_groups1" FOREIGN KEY ("code_categories_groups_id") REFERENCES "categories_groups" ("id");
ALTER TABLE "child" ADD CONSTRAINT "fk_child_parent1" FOREIGN KEY ("code_parent_id") REFERENCES "parent" ("id");
ALTER TABLE "child" ADD CONSTRAINT "fk_child_profile1" FOREIGN KEY ("default_custom_code_profile_id") REFERENCES "profile" ("id");
ALTER TABLE "device" ADD CONSTRAINT "device_code_child_id_fk" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "global_blocked_categories_groups_list" ADD CONSTRAINT "fk_global_blocked_categories_groups_list_copy1_categories_gro1" FOREIGN KEY ("code_categories_groups_id") REFERENCES "categories_groups" ("id");
ALTER TABLE "global_blocked_url_list" ADD CONSTRAINT "fk_global_blocked_url_list_url1" FOREIGN KEY ("code_url_id") REFERENCES "url" ("id");
ALTER TABLE "profile" ADD CONSTRAINT "fk_profile_parent1" FOREIGN KEY ("owner_code_parent_id") REFERENCES "parent" ("id");
ALTER TABLE "profile_categories_groups_list" ADD CONSTRAINT "fk_profile_categories_groups_list_profile1" FOREIGN KEY ("code_profile_id") REFERENCES "profile" ("id");
ALTER TABLE "profile_categories_groups_list" ADD CONSTRAINT "fk_profile_categories_groups_list_categories_groups1" FOREIGN KEY ("code_categories_groups_id") REFERENCES "categories_groups" ("id");
ALTER TABLE "profile_time_list" ADD CONSTRAINT "profile_block_time_list_code_profile_id" FOREIGN KEY ("code_profile_id") REFERENCES "profile" ("id");
ALTER TABLE "profile_time_list" ADD CONSTRAINT "fk_profile_time_list_child1" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "profile_url_list" ADD CONSTRAINT "profile_url_list_code_url_id_fk" FOREIGN KEY ("code_url_id") REFERENCES "url" ("id");
ALTER TABLE "profile_url_list" ADD CONSTRAINT "profile_url_list_code_profile_id_fk" FOREIGN KEY ("code_profile_id") REFERENCES "profile" ("id");
ALTER TABLE "tmp_access_categories_group_list" ADD CONSTRAINT "fk_tmp_access_categories_group_list_child1" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "tmp_access_categories_group_list" ADD CONSTRAINT "fk_tmp_access_categories_group_list_categories_groups1" FOREIGN KEY ("code_categories_groups_id") REFERENCES "categories_groups" ("id");
ALTER TABLE "tmp_access_control" ADD CONSTRAINT "fk_tmp_access_control_child1" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "tmp_access_url_list" ADD CONSTRAINT "fk_tmp_access_list_child1" FOREIGN KEY ("code_child_id") REFERENCES "child" ("id");
ALTER TABLE "tmp_access_url_list" ADD CONSTRAINT "fk_tmp_access_url_list_url1" FOREIGN KEY ("code_url_id") REFERENCES "url" ("id");
ALTER TABLE "webroot_overrides" ADD CONSTRAINT "fk_webroot_overrides_url1" FOREIGN KEY ("code_url_id") REFERENCES "url" ("id");
ALTER TABLE "webroot_overrides" ADD CONSTRAINT "fk_webroot_overrides_categories1" FOREIGN KEY ("code_categories_ext_id") REFERENCES "categories" ("ext_id");
