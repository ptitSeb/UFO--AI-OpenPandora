#include "FaceShader.h"
#include "ContentsFlagsValue.h"
#include "shared.h"
#include "debugging/debugging.h"
#include "shaderlib.h"

FaceShader::FaceShader (const std::string& shader, const ContentsFlagsValue& flags) :
	m_shader(shader), m_state(0), m_flags(flags), m_instanced(false), m_realised(false)
{
	captureShader();
}
FaceShader::~FaceShader ()
{
	releaseShader();
}

void FaceShader::instanceAttach ()
{
	m_instanced = true;
	m_state->incrementUsed();
}
void FaceShader::instanceDetach ()
{
	m_state->decrementUsed();
	m_instanced = false;
}

void FaceShader::captureShader ()
{
	ASSERT_MESSAGE(m_state == 0, "shader cannot be captured");
	if (!shader_valid(m_shader)) {
		globalErrorStream() << "brush face has invalid texture name: '" << m_shader << "'\n";
	}
	m_state = GlobalShaderCache().capture(m_shader);
	m_state->attach(*this);
}
void FaceShader::releaseShader ()
{
	ASSERT_MESSAGE(m_state != 0, "shader cannot be released");
	m_state->detach(*this);
	GlobalShaderCache().release(m_shader);
	m_state = 0;
}

void FaceShader::realise ()
{
	ASSERT_MESSAGE(!m_realised, "FaceTexdef::realise: already realised");
	m_realised = true;
	m_observers.forEach(FaceShaderObserverRealise());
}
void FaceShader::unrealise ()
{
	ASSERT_MESSAGE(m_realised, "FaceTexdef::unrealise: already unrealised");
	m_observers.forEach(FaceShaderObserverUnrealise());
	m_realised = false;
}

void FaceShader::attach (FaceShaderObserver& observer)
{
	m_observers.attach(observer);
	if (m_realised) {
		observer.realiseShader();
	}
}

void FaceShader::detach (FaceShaderObserver& observer)
{
	if (m_realised) {
		observer.unrealiseShader();
	}
	m_observers.detach(observer);
}

const std::string& FaceShader::getShader () const
{
	return m_shader;
}
void FaceShader::setShader (const std::string& name)
{
	if (m_instanced) {
		m_state->decrementUsed();
	}
	releaseShader();
	m_shader = name;
	captureShader();
	if (m_instanced) {
		m_state->incrementUsed();
	}
}

const ContentsFlagsValue& FaceShader::getFlags () const
{
	ASSERT_MESSAGE(m_realised, "FaceShader::getFlags: flags not valid when unrealised");
	return m_flags;
}
void FaceShader::setFlags (const ContentsFlagsValue& flags)
{
	ASSERT_MESSAGE(m_realised, "FaceShader::setFlags: flags not valid when unrealised");
	m_flags.assignMasked(flags);
}

Shader* FaceShader::state () const
{
	return m_state;
}

std::size_t FaceShader::width () const
{
	if (m_realised) {
		return m_state->getTexture().width;
	}
	return 1;
}
std::size_t FaceShader::height () const
{
	if (m_realised) {
		return m_state->getTexture().height;
	}
	return 1;
}
unsigned int FaceShader::shaderFlags () const
{
	if (m_realised) {
		return m_state->getFlags();
	}
	return 0;
}
