REVOKE CREATE ON SCHEMA public FROM PUBLIC;
CREATE ROLE backend LOGIN;
GRANT SELECT ON ALL TABLES IN SCHEMA public TO backend;
GRANT SELECT, UPDATE, INSERT, DELETE ON TABLE webroot_overrides TO backend;
GRANT USAGE ON SEQUENCE webroot_overrides_id_seq TO backend;
-- remember to set password with \password backend
