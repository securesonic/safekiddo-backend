DROP TABLE "webroot_overrides";

CREATE TABLE "webroot_overrides" (
"id" SERIAL PRIMARY KEY,
"address" text UNIQUE NOT NULL,
"code_categories_ext_id" integer NOT NULL REFERENCES "categories" ("ext_id")
);
