inline float Magnitude(Vector3* v) {
	float result = sqrt((v.x * v.x) + (v.y * v.y) + (v.z + v.z));
	return result;
}

inline Vector3 Normalize(Vector3* v) {
	Vector3 result;
	float magnitude = Magnitude(v);
	result.x = v.x / magnitude;
	result.y = v.y / magnitude;
	result.z = v.z / magnitude;
	return result;
}
